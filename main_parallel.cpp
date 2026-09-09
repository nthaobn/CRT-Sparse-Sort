#include <iostream>
#include <vector>
#include <cstdint>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <omp.h>

// Moduli hằng số cố định (Compile-time Constants)
constexpr uint64_t P1 = 3, P2 = 5, P3 = 7, P4 = 11, P5 = 13, P6 = 17, P7 = 19, P8 = 23, P9 = 29;
constexpr uint64_t M_TOTAL = P1 * P2 * P3 * P4 * P5 * P6 * P7 * P8 * P9; // ~3.23 tỷ (3,234,846,615)

// Hằng số kích thước phân đoạn tối đa của nửa gương
constexpr uint64_t S_MAX = (M_TOTAL / 2) / P9; // 1617423307 / 29 = 55,773,217 (Cần 26 bit)

// Cấu trúc đóng gói phần tử CRT
struct PackedElement {
    uint64_t packed_key; // Packed (Quotient 32-bit | Segment 27-bit | Local Offset 5-bit)
    uint64_t original_val;
};

// 1. Giai đoạn Phân rã Song song (Parallel Decomposition)
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

        // --- CƠ CHẾ LẬT GƯƠNG (MIRRORING OPTIMIZATION) ---
        if (norm_val <= M_TOTAL / 2) {
            // Nửa không lật gương
            M_bit = 0;
            uint64_t process_val = norm_val;
            uint64_t S_proc = process_val / P9;
            uint64_t L_proc = process_val % P9;

            S_key = S_proc;
            L_key = L_proc;
        } else {
            // Nửa có lật gương (Nén không gian dữ liệu)
            M_bit = 1;
            uint64_t process_val = M_TOTAL - norm_val;
            uint64_t S_proc = process_val / P9;
            uint64_t L_proc = process_val % P9;

            // Nghịch đảo tọa độ để khóa 64-bit bảo toàn tính tăng dần
            S_key = S_MAX - S_proc;
            L_key = (P9 - 1) - L_proc;
        }

        // Cấu trúc Packed Key 64-bit bảo toàn thứ tự:
        // Bit [32..63]: Quotient Q (32 bit)
        // Bit [31]:     Mirror Bit M (1 bit)
        // Bit [5..30]:  Segment Index S_key (26 bit -> 0x3FFFFFFULL)
        // Bit [0..4]:   Local Offset L_key (5 bit -> 0x1FULL)
        uint64_t key = (quotient << 32)
                     | (M_bit << 31)
                     | ((S_key & 0x3FFFFFFULL) << 5)
                     | (L_key & 0x1FULL);

        packed_vec[i] = {key, val};
    }
}

// 2. Giai đoạn Sắp xếp Lớp Radix Song song (Parallel Radix Sort)
void radix_sort_parallel(std::vector<PackedElement>& packed_vec) {
    size_t n = packed_vec.size();
    if (n == 0) return;

    std::vector<PackedElement> temp(n);
    int num_threads = omp_get_max_threads();
    omp_set_num_threads(num_threads); // Cố định số luồng để đảm bảo chia chunk đồng bộ

    for (int shift = 0; shift < 64; shift += 8) {
        // Bước 1: Đếm tần suất theo luồng (Thread-local histogram)
        std::vector<std::vector<size_t>> local_histogram(num_threads, std::vector<size_t>(256, 0));

        #pragma omp parallel num_threads(num_threads)
        {
            int thread_id = omp_get_thread_num();
            #pragma omp for schedule(static)
            for (size_t i = 0; i < n; ++i) {
                uint8_t byte_val = (packed_vec[i].packed_key >> shift) & 0xFF;
                local_histogram[thread_id][byte_val]++;
            }
        }

        // Bước 2: Tính toán vị trí ghi (Prefix Offset) chuẩn xác cho từng luồng
        std::vector<std::vector<size_t>> thread_offset(num_threads, std::vector<size_t>(256, 0));
        size_t current_offset = 0;
        for (int b = 0; b < 256; ++b) {
            for (int t = 0; t < num_threads; ++t) {
                thread_offset[t][b] = current_offset;
                current_offset += local_histogram[t][b];
            }
        }

        // Bước 3: Phân phối phần tử song song vào mảng tạm
        #pragma omp parallel num_threads(num_threads)
        {
            int thread_id = omp_get_thread_num();
            #pragma omp for schedule(static)
            for (size_t i = 0; i < n; ++i) {
                uint8_t byte_val = (packed_vec[i].packed_key >> shift) & 0xFF;
                size_t pos = thread_offset[thread_id][byte_val]++;
                temp[pos] = packed_vec[i];
            }
        }

        packed_vec.swap(temp);
    }
}

// 3. Giai đoạn Tái tạo Song song (Parallel Reconstruction)
void reconstruct_parallel(const std::vector<PackedElement>& packed_vec, std::vector<uint64_t>& output) {
    size_t n = packed_vec.size();

    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < n; ++i) {
        output[i] = packed_vec[i].original_val;
    }
}

int main() {
    std::cout << "=== CRT SPARSE SORT (PARALLEL VERSION - OPENMP) ===" << std::endl;
    std::cout << "Max CPU Threads: " << omp_get_max_threads() << std::endl;

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
    radix_sort_parallel(packed_vec);
    reconstruct_parallel(packed_vec, output);

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end_time - start_time;

    std::cout << "Thoi gian sap xep song song: " << duration.count() << " ms" << std::endl;

    // Kiểm tra tính đúng đắn ngay trong chương trình
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
