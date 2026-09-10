#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>

// Cấu trúc nút phẳng tối ưu L1/L2 Cache
struct NumberPoint {
    uint64_t original_value; // Giá trị số nguyên gốc (Hỗ trợ full 64-bit)
    uint64_t packed_key;     // Khóa đóng gói 64-bit: Q_key (28 bits) | S (31 bits) | L (5 bits)
};

// Hằng số hình học CRT
const uint64_t M_TOTAL = 3234846615ULL;
const uint64_t M_MID   = 1617423307ULL;
const uint64_t MAX_Q   = 0xFFFFFFFULL; // 28-bit mask cho hằng số Q_key (tối đa ~868 Trẻn)

// CẢI TIẾN 1 & 2: Mở phẳng vòng lặp phép chia & Đóng gói Khóa (Bit-Packing)
inline void computeCoordinatesAndPack(uint64_t x_calc, uint64_t original_x, uint64_t Q, bool is_upper, NumberPoint& pt) {
    pt.original_value = original_x;

    // 1. Unroll loop phép chia hằng số (Trình biên dịch tự động tối ưu thành nhân dịch bit / Magic Numbers)
    uint64_t q0 = x_calc / 3ULL;
    uint64_t q1 = x_calc / 5ULL;
    uint64_t q2 = x_calc / 7ULL;
    uint64_t q3 = x_calc / 11ULL;
    uint64_t q4 = x_calc / 13ULL;
    uint64_t q5 = x_calc / 17ULL;
    uint64_t q6 = x_calc / 19ULL;
    uint64_t q7 = x_calc / 23ULL;
    uint64_t q8 = x_calc / 29ULL;

    uint32_t S = 1 + static_cast<uint32_t>(q0 + q1 + q2 + q3 + q4 + q5 + q6 + q7 + q8);

    uint64_t b0 = 3ULL * q0;
    uint64_t b1 = 5ULL * q1;
    uint64_t b2 = 7ULL * q2;
    uint64_t b3 = 11ULL * q3;
    uint64_t b4 = 13ULL * q4;
    uint64_t b5 = 17ULL * q5;
    uint64_t b6 = 19ULL * q6;
    uint64_t b7 = 23ULL * q7;
    uint64_t b8 = 29ULL * q8;

    uint64_t max_bound = std::max({b0, b1, b2, b3, b4, b5, b6, b7, b8});
    uint32_t L = static_cast<uint32_t>(x_calc - max_bound);

    // 2. Tính Q_key cho phân loại gương
    uint64_t Q_key = is_upper ? (MAX_Q - Q) : Q;

    // 3. Đóng gói khóa 64-bit: Q_key (Bits 36..63) | S (Bits 5..35) | L (Bits 0..4)
    pt.packed_key = ((Q_key & MAX_Q) << 36) | (static_cast<uint64_t>(S) << 5) | (static_cast<uint64_t>(L) & 0x1FULL);
}

// Thuật toán Stable Radix Sort 8-bit trên khóa đóng gói 64-bit (Tối ưu an toàn Stack 2KB)
void stableRadixSort64(std::vector<NumberPoint>& vec) {
    if (vec.empty()) return;
    size_t n = vec.size();
    std::vector<NumberPoint> temp(n);

    // Duyệt 8 Pass (mỗi pass 8-bit = 256 Bins)
    for (int shift = 0; shift < 64; shift += 8) {
        size_t count[257] = {0};

        // Đếm tần suất
        for (size_t i = 0; i < n; ++i) {
            uint8_t digit = (vec[i].packed_key >> shift) & 0xFF;
            count[digit + 1]++;
        }

        // Tối ưu bỏ qua Pass nếu tất cả phần tử có cùng chữ số ở byte này (ví dụ các byte cao của Q = 0)
        uint8_t first_digit = (vec[0].packed_key >> shift) & 0xFF;
        if (count[first_digit + 1] == n) {
            continue;
        }

        // Prefix Sum
        for (int r = 0; r < 256; ++r) {
            count[r + 1] += count[r];
        }

        // Phân phối dữ liệu
        for (size_t i = 0; i < n; ++i) {
            uint8_t digit = (vec[i].packed_key >> shift) & 0xFF;
            temp[count[digit]++] = vec[i];
        }

        vec = temp;
    }
}

// Điều phối giải thuật tổng thể
std::vector<uint64_t> residueGeometrySparseSort(const std::vector<uint64_t>& input) {
    std::vector<NumberPoint> lower_group;
    std::vector<NumberPoint> upper_group;

    lower_group.reserve(input.size());
    upper_group.reserve(input.size());

    // CẢI TIẾN 3: Chia khối (Block Tiling) X = Q * M_TOTAL + R
    for (uint64_t x : input) {
        uint64_t Q = x / M_TOTAL;
        uint64_t R = x % M_TOTAL;

        NumberPoint pt;
        if (R <= M_MID) {
            computeCoordinatesAndPack(R, x, Q, false, pt);
            lower_group.push_back(pt);
        } else {
            uint64_t R_mirror = M_TOTAL - 1 - R;
            computeCoordinatesAndPack(R_mirror, x, Q, true, pt);
            upper_group.push_back(pt);
        }
    }

    // Sắp xếp độc lập 2 nhóm bằng 1 lượt Radix Sort 64-bit duy nhất
    stableRadixSort64(lower_group);
    stableRadixSort64(upper_group);

    // Hợp nhất kết quả bằng kỹ thuật Merge 2 con trỏ O(N)
    std::vector<uint64_t> result;
    result.reserve(input.size());

    size_t i = 0;
    size_t j = upper_group.size(); // Nhóm trên đọc ngược từ cuối về đầu

    while (i < lower_group.size() && j > 0) {
        uint64_t val_lower = lower_group[i].original_value;
        uint64_t val_upper = upper_group[j - 1].original_value;

        if (val_lower <= val_upper) {
            result.push_back(val_lower);
            i++;
        } else {
            result.push_back(val_upper);
            j--;
        }
    }

    while (i < lower_group.size()) {
        result.push_back(lower_group[i++].original_value);
    }
    while (j > 0) {
        result.push_back(upper_group[--j].original_value);
    }

    return result;
}

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
    std::vector<uint64_t> sorted_data = residueGeometrySparseSort(input_data);
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
