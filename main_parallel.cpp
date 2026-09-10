#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <omp.h>

// --- CÁC HẰNG SỐ TOÁN HỌC GỐC VÀ CẤU TRÚC DỮ LIỆU v1.0.0 ---
constexpr uint64_t M_TOTAL = 3234846615ULL;
constexpr uint64_t M_MID   = 1617423307ULL;
constexpr uint64_t MAX_Q   = (1ULL << 28) - 1; // Mask 28-bit cho Q_key (MAX_Q)

struct NumberPoint {
    uint64_t val;
    uint64_t packed_key;
};

// 1. TÍNH TỌA ĐỘ HÌNH HỌC (S, L) TRÊN 9 TRỤC SỐ NGUYÊN TỐ (BẢO TOÀN v1.0.0)
inline std::pair<uint64_t, uint64_t> compute_coordinates_v100(uint64_t R) {
    uint64_t q0 = R / 3,  b0 = 3 * q0;
    uint64_t q1 = R / 5,  b1 = 5 * q1;
    uint64_t q2 = R / 7,  b2 = 7 * q2;
    uint64_t q3 = R / 11, b3 = 11 * q3;
    uint64_t q4 = R / 13, b4 = 13 * q4;
    uint64_t q5 = R / 17, b5 = 17 * q5;
    uint64_t q6 = R / 19, b6 = 19 * q6;
    uint64_t q7 = R / 23, b7 = 23 * q7;
    uint64_t q8 = R / 29, b8 = 29 * q8;

    uint64_t S = 1 + (q0 + q1 + q2 + q3 + q4 + q5 + q6 + q7 + q8);
    uint64_t max_b = std::max({b0, b1, b2, b3, b4, b5, b6, b7, b8});
    uint64_t L = R - max_b;

    return {S, L};
}

// 2. STABLE RADIX SORT 8-PASS TRÊN MẢNG PHẲNG KHÓA ĐÓNG GÓI 64-BIT
void radix_sort_points(std::vector<NumberPoint>& vec) {
    if (vec.empty()) return;
    size_t n = vec.size();
    std::vector<NumberPoint> temp(n);

    for (int shift = 0; shift < 64; shift += 8) {
        size_t count[257] = {0};

        for (size_t i = 0; i < n; ++i) {
            uint8_t digit = (vec[i].packed_key >> shift) & 0xFF;
            count[digit + 1]++;
        }

        uint8_t first_digit = (vec[0].packed_key >> shift) & 0xFF;
        if (count[first_digit + 1] == n) {
            continue;
        }

        for (int r = 0; r < 256; ++r) {
            count[r + 1] += count[r];
        }

        for (size_t i = 0; i < n; ++i) {
            uint8_t digit = (vec[i].packed_key >> shift) & 0xFF;
            temp[count[digit]++] = vec[i];
        }

        vec.swap(temp);
    }
}

// 3. TÌM KHÓA CẮT CHO PARALLEL MERGE BẰNG BINARY SEARCH
size_t find_split_A(const std::vector<NumberPoint>& A,
                    const std::vector<NumberPoint>& B,
                    size_t K) {
    size_t N_A = A.size();
    size_t N_B = B.size();

    size_t low = (K > N_B) ? (K - N_B) : 0;
    size_t high = std::min(N_A, K);

    while (low < high) {
        size_t i = low + (high - low) / 2;
        size_t k = K - i;

        if (A[i].val < B[N_B - k].val) {
            low = i + 1;
        } else {
            high = i;
        }
    }
    return low;
}

// 4. BƯỚC HỢP NHẤT PARALLEL 2-POINTER MERGE
void parallel_merge_v100(const std::vector<NumberPoint>& A,
                         const std::vector<NumberPoint>& B,
                         std::vector<uint64_t>& output) {
    size_t N_A = A.size();
    size_t N_B = B.size();
    size_t N = N_A + N_B;
    output.resize(N);

    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        int num_threads = omp_get_num_threads();

        size_t chunk_size = (N + num_threads - 1) / num_threads;
        size_t out_start = std::min((size_t)tid * chunk_size, N);
        size_t out_end   = std::min(out_start + chunk_size, N);

        if (out_start < out_end) {
            size_t i = find_split_A(A, B, out_start);
            size_t k = out_start - i;
            size_t j = N_B - k;       // Con trỏ j duyệt từ cuối mảng B lên đầu

            size_t out_idx = out_start;
            while (out_idx < out_end) {
                bool pick_A = false;
                if (i < N_A && j > 0) {
                    if (A[i].val <= B[j - 1].val) {
                        pick_A = true;
                    }
                } else if (i < N_A) {
                    pick_A = true;
                }

                if (pick_A) {
                    output[out_idx++] = A[i++].val;
                } else {
                    output[out_idx++] = B[--j].val;
                }
            }
        }
    }
}

// 5. ĐIỀU PHỐI THUẬT TOÁN SONG SONG SONG v1.0.0
std::vector<uint64_t> crtSparseSortV100Parallel(const std::vector<uint64_t>& input) {
    size_t N = input.size();
    int max_threads = omp_get_max_threads();

    std::vector<std::vector<NumberPoint>> local_lower(max_threads);
    std::vector<std::vector<NumberPoint>> local_upper(max_threads);

    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        local_lower[tid].reserve(N / max_threads / 2 + 100);
        local_upper[tid].reserve(N / max_threads / 2 + 100);

        #pragma omp for schedule(static)
        for (size_t idx = 0; idx < N; ++idx) {
            uint64_t x = input[idx];
            uint64_t Q = x / M_TOTAL;
            uint64_t R = x % M_TOTAL;

            if (R <= M_MID) {
                auto coord = compute_coordinates_v100(R);
                uint64_t S = coord.first;
                uint64_t L = coord.second;
                uint64_t key = ((Q & MAX_Q) << 36) | (S << 5) | (L & 0x1F);
                local_lower[tid].push_back({x, key});
            } else {
                uint64_t R_mirror = M_TOTAL - 1 - R;
                auto coord_mirror = compute_coordinates_v100(R_mirror);
                uint64_t S = coord_mirror.first;
                uint64_t L = coord_mirror.second;
                uint64_t Q_key = MAX_Q - Q;
                uint64_t key = ((Q_key & MAX_Q) << 36) | (S << 5) | (L & 0x1F);
                local_upper[tid].push_back({x, key});
            }
        }
    }

    std::vector<size_t> lower_offsets(max_threads + 1, 0);
    std::vector<size_t> upper_offsets(max_threads + 1, 0);

    for (int t = 0; t < max_threads; ++t) {
        lower_offsets[t + 1] = lower_offsets[t] + local_lower[t].size();
        upper_offsets[t + 1] = upper_offsets[t] + local_upper[t].size();
    }

    std::vector<NumberPoint> lower_group(lower_offsets[max_threads]);
    std::vector<NumberPoint> upper_group(upper_offsets[max_threads]);

    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        std::copy(local_lower[tid].begin(), local_lower[tid].end(),
                  lower_group.begin() + lower_offsets[tid]);
        std::copy(local_upper[tid].begin(), local_upper[tid].end(),
                  upper_group.begin() + upper_offsets[tid]);
    }

    #pragma omp parallel sections
    {
        #pragma omp section
        {
            radix_sort_points(lower_group);
        }
        #pragma omp section
        {
            radix_sort_points(upper_group);
        }
    }

    std::vector<uint64_t> sorted_data;
    parallel_merge_v100(lower_group, upper_group, sorted_data);
    return sorted_data;
}

// --- HÀM MAIN THEO ĐÚNG TIÊU CHUẨN ĐỌC GHI FILE VÀ IN LOG ---
int main() {
    std::cout << "Dang mo file crt.inp..." << std::endl;
    std::ifstream infile("crt.inp");
    if (!infile.is_open()) {
        std::cerr << "Loi: Khong the mo file crt.inp!" << std::endl;
        return 1;
    }

    size_t n;
    if (!(infile >> n)) return 0;

    std::vector<uint64_t> input_data;
    input_data.reserve(n);

    uint64_t temp_val;
    for (size_t i = 0; i < n; ++i) {
        if (infile >> temp_val) {
            input_data.push_back(temp_val);
        }
    }
    infile.close();
    std::cout << "Da nap xong " << input_data.size() << " phan tu vao RAM." << std::endl;

    auto start = std::chrono::high_resolution_clock::now();
    std::vector<uint64_t> sorted_data = crtSparseSortV100Parallel(input_data);
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Thoi gian thuc thi tinh toan: " << duration.count() << " ms" << std::endl;

    // Kiểm tra tính đúng đắn
    bool is_correct = std::is_sorted(sorted_data.begin(), sorted_data.end());
    if (is_correct) {
        std::cout << "Kiem tra: MANG DA DUOC SAP XEP CHINH XAC TANG DAN!" << std::endl;
    } else {
        std::cout << "Kiem tra: CANH BAO - Mang van con sai thu tu!" << std::endl;
    }

    // Ghi kết quả xuất ra file crt.out
    std::ofstream outfile("crt.out");
    if (!outfile.is_open()) {
        std::cerr << "Loi: Khong the tao file crt.out!" << std::endl;
        return 1;
    }

    outfile << n << "\n";
    for (size_t i = 0; i < sorted_data.size(); ++i) {
        outfile << sorted_data[i] << "\n";
    }
    outfile.close();

    std::cout << "Da xuat ket qua ra file crt.out hoan tat!" << std::endl;
    return 0;
}
