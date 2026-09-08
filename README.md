# CRT Sparse Sort (Thanh Ha Algorithm)

A domain-specific $O(N)$ non-comparative sorting framework based on the Chinese Remainder Theorem (CRT) and 2D Coordinate Space Mapping $(S, L)$. Designed for high-performance microarchitectures, pipeline hardware accelerators (FPGA/ASIC), and cyclic signal processing systems (DSP/Radar).

---

## 📁 Repository Structure

| File Name | Description |
| :--- | :--- |
| **`ThanhHaAlgorithm.pdf`** | Full scientific paper manuscript in English (IEEEtran format). |
| **`ThanhHaAlgorithm_Vietnamese.pdf`** | Full scientific paper manuscript in Vietnamese (IEEEtran format). |
| **`main.cpp`** | C++11 implementation of the CRT Sparse Sort ($O(N)$ non-comparative algorithm). |
| **`main_gen_input.cpp`** | Utility script to generate test datasets (`crt.inp`) with 64-bit random integers. |

---

## 🚀 Key Theoretical Features

* **2D Coordinate Mapping $(S, L)$**: Decomposes 1D integers into segment index $S$ and local phase $L$, preserving 100% monotonic algebraic order without requiring pairwise comparison operators.
* **Mirror Symmetry Split**: Reflects residue values across the CRT period midpoint ($M_{\text{MID}}$), saving 50% coordinate storage space while keeping $O(N)$ unmirroring overhead.
* **64-bit Bit-Packing Key**: Packs Quotient $Q$, Segment $S$, and Local Offset $L$ into a single `uint64_t` variable, unifying multi-criteria sorting into a single flat-array Radix Sort pass.
* **Branch-Free Execution**: Eliminates control-flow branches inside the core loop, ensuring deterministic execution time and optimization for L1 Cache locality.

---

## 🛠️ Quick Start & Usage

### 1. Generate Input Data
Compile and run `main_gen_input.cpp` to create the input data file (`crt.inp`):

```bash
g++ -O3 main_gen_input.cpp -o gen_input
./gen_input
```

### 2. Execute CRT Sparse Sort
Compile and run `main.cpp` to sort the dataset and produce `crt.out`:

```bash
g++ -O3 main.cpp -o crt_sort
./crt_sort
```

---

## 📖 Citation

If you use this algorithm or reference this work in your research, please cite:

```bibtex
@article{nguyen2026crtspsort,
  title={CRT Sparse Sort (Thanh Ha Algorithm): A Non-Comparative
         Sorting Framework via Chinese Remainder Theorem and 2D
         Coordinate Mapping for Hardware Accelerators},
  author={Nguyen, Hao T.},
  year={2026},
  publisher={GitHub},
  howpublished={\url{[https://github.com/nthaobn/CRT-Sparse-Sort](https://github.com/nthaobn/CRT-Sparse-Sort)}}
}
```

---

**Author:** Hao T. Nguyen (Nguyễn Thanh Hào)  
*Dedicated to my beloved daughter, Nguyen Ngoc Thanh Ha.*
