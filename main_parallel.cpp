#include <iostream>
#include <vector>
#include <cstdint>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <omp.h>

// Moduli hằng số cố định
constexpr uint64_t P1 = 3, P2 = 5, P3 = 7, P4 = 11, P5 = 13, P6 = 17, P7 = 19, P8 = 23, P9 = 29;
constexpr uint64_t M_TOTAL = P1 * P2 * P3 * P4 * P5 * P6 * P7 * P8 * P9; // ~3.23 tỷ

// Hằng số kích thước phân đoạn tối đa của nửa gương
constexpr uint64_t S_MAX = (M_TOTAL / 2) / P9; // 1617423307 / 29 = 55,773,217 (Cần 26 bit)

// Cấu trúc đóng gói phần tử CRT
struct PackedElement {
    uint64_t packed_key;
    uint64_t original_val;
};

// Cấu trúc dữ liệu theo luồng được căn lề 64-byte (Cache Line) để triệt tiêu False Sharing
struct alignas(64) ThreadLocalData {
    size_t count[256];
    size_t offset[256];
};

// 1. Giai đoạn Phân rã Song song + Lật gương
void decompose_parallel(const std::vector<uint64_t>& input, std::vector<PackedElement>& packed_vec) {
    size_t n = input.size();

    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < n; ++i) {
        uint64_t val = input[i];
        uint64_t quotient = val / M_TOTAL;
        uint64_t norm_val = val % M_TOTAL;

        uint64_t M_bit = 0;
        uint64_t S_key = 0;
        uint64_t L_key = 0;

        if (norm_val <= M_TOTAL / 2) {
            M_bit = 0;
            uint64_t process_val = norm_val;
            S_key = process_val / P9;
            L_key = process_val % P9;
        } else {
            M_bit = 1;
            uint64_t process_val = M_TOTAL - norm_val;
            S_key = S_MAX - (process_val / P9);
            L_key = (P9 - 1) - (process_val % P9);
        }

        uint64_t key = (quotient << 32)
                     | (M_bit << 31)
                     | ((S_key & 0x3FFFFFFULL) << 5)
                     | (L_key & 0x1FULL);

        packed_vec[i] = {key, val};
    }
}

// 2. Sắp xếp Radix Song song tối ưu
void radix_sort_parallel_optimized(std::vector<PackedElement>& packed_vec, int num_threads) {
    size_t n = packed_vec.size();
    if (n == 0) return;

    std::vector<PackedElement> temp(n);

    // Cấp phát bộ nhớ 1 lần duy nhất cho tất cả các bước Radix
    std::vector<ThreadLocalData> thread_data(num_threads);

    for (int shift = 0; shift < 64; shift += 8) {
        // Clear đếm tần suất
        #pragma omp parallel num_threads(num_threads)
        {
            int tid = omp_get_thread_num();
            for (int b = 0; b < 256; ++b) {
                thread_data[tid].count[b] = 0;
            }
        }

        // Bước 1: Đếm tần suất song song
        #pragma omp parallel num_threads(num_threads)
        {
            int tid = omp_get_thread_num();
            #pragma omp for schedule(static)
            for (size_t i = 0; i < n; ++i) {
                uint8_t byte_val = (packed_vec[i].packed_key >> shift) & 0xFF;
                thread_data[tid].count[byte_val]++;
            }
        }

        // Bước 2: Tính toán offset cộng dồn
        size_t current_offset = 0;
        for (int b = 0; b < 256; ++b) {
            for (int t = 0; t < num_threads; ++t) {
                thread_data[t].offset[b] = current_offset;
                current_offset += thread_data[t].count[b];
            }
        }

        // Bước 3: Phân phối phần tử song song vào mảng tạm
        #pragma omp parallel num_threads(num_threads)
        {
            int tid = omp_get_thread_num();
            #pragma omp for schedule(static)
            for (size_t i = 0; i < n; ++i) {
                uint8_t byte_val = (packed_vec[i].packed_key >> shift) & 0xFF;
                size_t pos = thread_data[tid].offset[byte_val]++;
                temp[pos] = packed_vec[i];
            }
        }

        packed_vec.swap(temp);
    }
}

// 3. Tái tạo dữ liệu
void reconstruct_parallel(const std::vector<PackedElement>& packed_vec, std::vector<uint64_t>& output) {
    size_t n = packed_vec.size();
    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < n; ++i) {
        output[i] = packed_vec[i].original_val;
    }
}

int main() {
    std::cout << "=== CRT SPARSE SORT (OPTIMIZED PARALLEL) ===" << std::endl;

    // Giới hạn luồng để tối ưu băng thông RAM (Ví dụ 4 luồng cho CPU 6-Core)
    int max_hardware_threads = omp_get_max_threads();
    int target_threads = std::min(4, max_hardware_threads);
    omp_set_num_threads(target_threads);

    std::cout << "Hardware Threads: " << max_hardware_threads
              << " | Active Sorting Threads: " << target_threads << std::endl;

    std::ifstream infile("crt.inp");
    if (!infile.is_open()) {
        std::cerr << "Loi: Khong the mo file crt.inp!" << std::endl;
        return 1;
    }

    size_t n;
    if (!(infile >> n)) return 1;

    std::vector<uint64_t> input(n);
    for (size_t i = 0; i < n; ++i) {
        infile >> input[i];
    }
    infile.close();

    std::cout << "Dang sap xep " << n << " phan tu..." << std::endl;

    std::vector<PackedElement> packed_vec(n);
    std::vector<uint64_t> output(n);

    auto start_time = std::chrono::high_resolution_clock::now();

    decompose_parallel(input, packed_vec);
    radix_sort_parallel_optimized(packed_vec, target_threads);
    reconstruct_parallel(packed_vec, output);

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end_time - start_time;

    std::cout << "Thoi gian sap xep song song: " << duration.count() << " ms" << std::endl;

    bool is_correct = std::is_sorted(output.begin(), output.end());
    if (is_correct) {
        std::cout << "Kiem tra: MANG DA DUOC SAP XEP CHINH XAC TANG DAN!" << std::endl;
    } else {
        std::cout << "Kiem tra: CANH BAO - Mang van con sai thu tu!" << std::endl;
    }

    std::ofstream outfile("crt.out");
    outfile << n << "\n";
    for (size_t i = 0; i < n; ++i) {
        outfile << output[i] << "\n";
    }
    outfile.close();

    std::cout << "Da ghi ket qua sap xep vao file crt.out." << std::endl;
    return 0;
}
