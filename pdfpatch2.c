#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

static bool validate(uint8_t *base, unsigned int len) {
    bool large_int = false;
    uint8_t *ptr = base, *end = base + len;
    bool okay_1st = false, okay_2nd = false;
#define read(len) if(end - ptr < len) return true; ptr += len
    // this is basically a copy of the original code
    while(1) {
        int32_t value = -1;
        uint8_t *p = ptr;
        read(1);
        if(p[0] == 255) {
            read(4);
            value = (p[1] << 24) | (p[2] << 16) | (p[3] << 8) | p[4];
            if(value > 32000 || value < -32000) {
                large_int = true;
            } else if(!large_int) {
                value <<= 16;
            }
        } else if(p[0] >= 32) {
            if(p[0] < 247) {
                value = (int32_t) p[0] - 139;
            } else {
                read(1);
                if(p[0] < 251) {
                    value = (((int32_t) p[0] - 247) << 8) + p[1] + 108;
                } else {
                    value = -((((int32_t) p[0] - 251) << 8) + p[1] + 108);
                }
            }
            if(!large_int) value <<= 16;
        } else {
            large_int = false;
            if(p[0] == 12) {
                read(1);
                if(p[1] == 16) {
                    // callothersubr
                    if(!okay_2nd) {
                        return false;
                    }
                }
            }
        }
        okay_2nd = okay_1st;
        okay_1st = value >= 0;
    }
#undef read
}
#ifdef TESTER
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
int main(int argc, char **argv) {
    int fd = open(argv[1], O_RDONLY);
    assert(fd != -1);
    off_t end = lseek(fd, 0, SEEK_END);
    void *data = mmap(NULL, (size_t) end, PROT_READ, MAP_SHARED, fd, 0);
    assert(data != MAP_FAILED);

    printf("%d\n", validate(data, (unsigned int) end));
    return 0;
}
#else
#include <substrate.h>
#include <mach-o/dyld.h>
static int (*real)(void *decoder, uint8_t *base, unsigned int len);
static int mine(void *decoder, uint8_t *base, unsigned int len) {
    //fprintf(stderr, "pp2 mine len=%u\n", len);
    if(validate(base, len)) {
        return real(decoder, base, len);
    } else {
        fprintf(stderr, "Possibly evil font detected in t1_decoder_parse_charstrings.\n");
        return 0xa0; // Syntax_Error
    }
}
static bool did_hook;
struct mach_header;
static void pp2_hook(const struct mach_header *mh, intptr_t vmaddr_slide) {
    if(did_hook) return;
    struct nlist nl[2];
    memset(nl, 0, sizeof(nl));
    nl[0].n_un.n_name = "_t1_decoder_parse_charstrings";
    if(nlist("/System/Library/Frameworks/CoreGraphics.framework/Resources/libCGFreetype.A.dylib", nl)) {
        //fprintf(stderr, "WARNING: Could not find t1_decoder_parse_charstrings to hook it.\n");
        return;
    }
    did_hook = true;
    //fprintf(stderr, "Well i did find it.\n");
    uintptr_t value = nl[0].n_value | ((nl[0].n_desc & N_ARM_THUMB_DEF) ? 1 : 0);
    MSHookFunction((void *) value, mine, (void **) &real);
}

__attribute__((constructor))
void pp2_init() {
    _dyld_register_func_for_add_image(pp2_hook);
    pp2_hook(NULL, 0);
}
#endif
