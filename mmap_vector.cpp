#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <sys/resource.h>

#include <iostream>
#include <cstdint>
#include <vector>


std::uintptr_t as_integer(void * addr) {
  return reinterpret_cast<std::uintptr_t>(addr);
}

void * as_pointer(std::uintptr_t addr) {
  return reinterpret_cast<void *>(addr);
}

void * page_begin(void * addr) {
  auto pagesize = sysconf(_SC_PAGE_SIZE);
  return as_pointer(as_integer(addr) & pagesize);
}

void * next_page(void * addr) {
  auto pagesize = sysconf(_SC_PAGE_SIZE);
  return as_pointer(((as_integer(addr) + pagesize - 1) / pagesize) * pagesize);
}

void * mmap_reserve(size_t n) {
  std::cout << "mmap_reserve: n = " << n << std::endl;
  void * addr = mmap(nullptr, n, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if(addr == MAP_FAILED) {
    throw std::runtime_error("Failed to mmap, errno: " + std::to_string(errno));
  }

  return addr;
}

void mmap_grow(void * addr, size_t n_old, size_t n_new) {
  void *begin = next_page((char*) addr + n_old);
  std::ptrdiff_t dn = (char *)addr + n_new - (char *)(begin);

  if (dn > 0) {
    if(mprotect(begin, n_new, PROT_WRITE | PROT_READ) == -1) {
      throw std::runtime_error("Failed mprotect: " + std::to_string(errno));
    }
  }
}

void mmap_shrink(void * addr, size_t n_old, size_t n_new) {
  void * begin = next_page((char*)addr + n_new);
  std::ptrdiff_t dn = (char*) addr + n_old - (char*)(begin);
  if(dn > 0) {
    if(madvise(begin, dn, MADV_DONTNEED) == -1) {
      throw std::runtime_error("Failed madvise: " + std::to_string(errno));
    }

    if(mprotect(begin, dn, PROT_NONE) == -1) {
      throw std::runtime_error("Failed mprotect: " + std::to_string(errno));
    }
  }
}

void mmap_resize(void * addr, size_t n_old, size_t n_new) {
  if(n_old < n_new) {
    mmap_grow(addr, n_old, n_new);
  }
  else if(n_old > n_new) {
    mmap_shrink(addr, n_old, n_new);
  }
}

void mmap_release(void * addr, size_t n) {
  auto status = munmap(addr, n);
  if(status == -1) {
    throw std::runtime_error("Failed to unmap.");
  }
}

int main(int argc, char* argv[]) {
  size_t n_max = 1ul << 40;

  auto * x = (double*) mmap_reserve(n_max * sizeof(double));
  std::cout << "x = " << x << std::endl;

  size_t n = 1ul << 30;
  size_t n_capacity = 0ul;
  size_t grow_by = 1024;
  for(size_t i = 0; i < n; ++i) {
    if(i >= n_capacity) {
      size_t n_old = n_capacity * sizeof(double);
      size_t n_new = (n_capacity + grow_by) * sizeof(double);
      mmap_resize(x, n_old, n_new);
      n_capacity += grow_by;
    }

    x[i] = double(i);
  }

  std::cout << x[0] << "\n";
  std::cout << x[n - 1] << "\n";
  std::cout << "x = " << x << std::endl;

  mmap_release(x, n_max*sizeof(double));

  return 0;
}

