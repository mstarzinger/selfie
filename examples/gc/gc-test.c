// testing selfie's garbage collector

// to compile the garbage collector as library use flag "-gc" with selfie.h
// e.g. ./selfie -gc selfie.h examples/gc/gc-test.c -m 1

// since selfie implements a conservative garbage collector
// we disguise pointers for validation as integers with an offset
uint64_t check_offset = 4096;
uint64_t heap_start = 0;

// comparison values (to see whether a check has succeeded)
uint64_t validation_address_1 = 0;
uint64_t validation_address_2 = 0;
uint64_t validation_address_3 = 0;
uint64_t validation_address_4 = 0;
uint64_t validation_address_5 = 0;

// global variables (pointers to gc created objects)
uint64_t* x = (uint64_t*) 0;
uint64_t* y = (uint64_t*) 0;

// simple function allocating memory to demonstrate stack collection
void do_stuff() {
  uint64_t* z;

  z = gc_malloc(8); // object 5
  if (((uint64_t) z) != validation_address_4 - check_offset) {
    print("test 4 failed (local variable inside function)!\n");
    exit(1);
  }

  // return -> free(object 5)
}

int main(int argc, char** argv) {
  // local variables (pointers to gc created objects)
  uint64_t* z;
  uint64_t* w;

  // this example should demonstrate the capabilities of the garbage collector.
  // for simplicity we configure it to collect before every gc_malloc.
  turn_on_gc_library(0, " gc-test");

  // exit with error code 1 if gc is not available
  if (USE_GC_LIBRARY != GC_ENABLED)
    exit(1);

  // initialize library (power_of_2 table, etc.)
  init_library();

  // determine heap start to calculate all expected addresses

  // assert: gc_malloc(0) fetches the current program break
  heap_start = (uint64_t) gc_malloc(0) + check_offset;

  // note: the gc library stores metadata and object in the same heap (order: metadata -> object)
  // therefore, we need to consider both object and metadata size when calculating the expected addresses

  // test case 1: allocate object and assign ptr to global variable (-> data segment)
  validation_address_1 = heap_start + GC_METADATA_SIZE;

  // test case 2: allocate object and assign ptr to the first object (-> heap)
  validation_address_2 = validation_address_1 + 8 + GC_METADATA_SIZE;

  // test case 3: reuse (i.e. alloc -> unassign -> alloc)
  validation_address_3 = validation_address_2 + 8 + GC_METADATA_SIZE;

  // test case 4: call function (which allocates memory and assigns it to local variable)
  //              -> reuse (since variable of function goes out of scope)
  validation_address_4 = validation_address_3 + 8 + GC_METADATA_SIZE;

  // test case 5: allocate memory of size not in free list (should result in a new alloc)
  validation_address_5 = validation_address_4 + 8 + GC_METADATA_SIZE;

  // test case 6: unassign global variable (whose memory contains pointer to other heap address)
  // the first gc_malloc zeroes and frees the memory of x, therefore unreferencing y
  // the second gc_malloc reuses the memory of y afterwards.
  // validation addresses of test case 1 and 2 are used

  // heap layout (not considering metadata and check offsets):
  // +-----------+
  // |           |
  // | object 7  |
  // +-----------+    +-----------+  = validation_address_5
  // | object 5  | -> | object 6  |
  // +-----------+    +-----------+  = validation_address_4
  // | object 3  | -> | object 4  |
  // +-----------+    +-----------+  = validation_address_3
  // | object 2  | -> | object 9  |
  // +-----------+    +-----------+  = validation_address_2
  // | object 1  | -> | object 8  |
  // +-----------+    +-----------+  = validation_address_1

  // test cases

  // --- test 1 ---
  x = gc_malloc(8); // object 1
  if (((uint64_t) x) != validation_address_1 - check_offset) {
    printf("0x%08lX - 0x%lX\n", (uint64_t) x, validation_address_1 - check_offset);
    print("test 1 failed!\n");
    exit(1);
  }

  // --- test 2 ---
  *x = (uint64_t) gc_malloc(8); // object 2
  if (*x != validation_address_2 - check_offset) {
    print("test 2 failed!\n");
    exit(1);
  }

  // --- test 3 ---
  y = gc_malloc(8); // object 3
  if (((uint64_t) y) != validation_address_3 - check_offset) {
    print("test 3 failed (first allocation)!\n");
    exit(1);
  }

  y = (uint64_t*) 0; // = free(object 3)

  y = gc_malloc(8); // object 4
  if (((uint64_t) y) != validation_address_3 - check_offset) {
    print("test 3 failed (reuse)! make sure gc period is set to 0!\n");
    exit(1);
  }

  // --- test 4 ---
  do_stuff(); // object 5 (inside function)

  z = gc_malloc(8); // object 6
  if (((uint64_t) z) != validation_address_4 - check_offset) {
    print("test 4 failed (local variable)! make sure gc period is set to 0!\n");
    exit(1);
  }

  // --- test 5 ---
  z = (uint64_t*) 0; // = free(object 6)

  z = gc_malloc(16); // object 7
  if (((uint64_t) z) != validation_address_5 - check_offset) {
    print("test 5 failed!\n");
    exit(1);
  }

  // --- test 6 ---
  x = (uint64_t*) 0; // = free(object 1), free(object 2)

  w = gc_malloc(8); // object 8
  if (((uint64_t) w) != validation_address_1 - check_offset) {
    print("test 6 failed! make sure gc period is set to 0!\n");
    exit(1);
  }

  x = gc_malloc(8); // object 9
  if (((uint64_t) x) != validation_address_2 - check_offset) {
    print("test 6 failed! make sure gc period is set to 0!\n");
    exit(1);
  }

  print_gc_profile((uint64_t*) 0);
}