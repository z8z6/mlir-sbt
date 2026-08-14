#include <stdint.h>

extern void translated_block(uint64_t *state);

int main(void) {
  uint64_t state[65] = {0};
  uint64_t guest_stack[128];
  state[7] = (uint64_t)(guest_stack + 128);
  translated_block(state);
  return (int)state[0];
}
