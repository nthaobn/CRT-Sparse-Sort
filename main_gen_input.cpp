#include <iostream>
#include <fstream>
#include <random>
#include <chrono>

int main() {
    // Khởi tạo file ghi dữ liệu
    std::ofstream outfile("crt.inp");
    if (!outfile.is_open()) {
        std::cerr << "Loi: Khong the tao file crt.inp!" << std::endl;
        return 1;
    }

    // Định nghĩa số lượng phần tử: 1 triệu số
    const size_t N = 1000000;
    outfile << N << "\n";

    // Trình sinh số ngẫu nhiên phần cứng
    std::mt19937_64 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<uint64_t> dist(0, 2000000000000ULL); // Khoảng giá trị từ 0 đến 2 nghìn tỷ

    // Sinh dữ liệu và ghi liên tục vào file
    for (size_t i = 0; i < N; ++i) {
        outfile << dist(rng) << (i == N - 1 ? "" : " ");
    }

    outfile << "\n";
    outfile.close();

    std::cout << "Da tao thanh cong file crt.inp chua 1 trieu so nguyen ngau nhien!" << std::endl;
    return 0;
}
