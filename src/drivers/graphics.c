/**
 * CGA Framebuffer driver with SSFN font and 24 bit color.
 * https://wiki.osdev.org/Scalable_Screen_Font 
 * functions: graphics_init(), fb_plotpixel(), fb_changebg(), fb_printchar(), fb_print(), fb_print_color(), fb_print_bits()
 **/



#include <thirdparty/stivale2.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <include/framebuffer.h>
#include <include/stivale_tag.h>
#include <lib/printf.h>
#include <lib/string.h> 
#include <thirdparty/ssfn.h>

#define CHARACTER_WIDTH 8
#define CHARACTER_HEIGHT 16 

uint64_t framebuffer_addr;
struct fb_struct fb;

extern uint8_t _binary_sfn_fonts_unifont_sfn_start;

void fb_changebg(uint32_t color);
void graphics_init(struct stivale2_struct *stivale2_struct, uint32_t bgcolor, uint32_t fgcolor){
    struct stivale2_struct_tag_framebuffer* framebuffer_tag;
    framebuffer_tag = (struct stivale2_struct_tag_framebuffer*) stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_FRAMEBUFFER_ID);
    if (framebuffer_tag == NULL) { // check if framebuffer tag is there
        for (;;) {
            asm ("hlt");
        }
    }
    dprintf("Framebuffer found at 0x%x\n", framebuffer_tag->framebuffer_addr);

    fb.addr = framebuffer_tag->framebuffer_addr;
    fb.width = framebuffer_tag->framebuffer_width;
    fb.height = framebuffer_tag->framebuffer_height;
    fb.bpp = framebuffer_tag->framebuffer_bpp;
    fb.pitch = framebuffer_tag->framebuffer_pitch;

    ssfn_src = (ssfn_font_t*) &_binary_sfn_fonts_unifont_sfn_start;
    ssfn_dst.ptr = (uint8_t*) fb.addr;
    ssfn_dst.w = fb.width;
    ssfn_dst.h = fb.height;
    ssfn_dst.p = fb.pitch;
    ssfn_dst.x = 0;
    ssfn_dst.y = 0;


    fb_changebg(bgcolor);
    ssfn_dst.fg = fgcolor;

    // fb_print("(Black screen with text; The sound of buzzing bees can be heard) Narrator: According to all known laws of aviation, there is no way a bee should be able to fly. Its wings are too small to get its fat little body off the ground. The bee, of course, flies anyway because bees don't care what humans think is impossible. (Barry is picking out a shirt) Barry: Yellow, black. Yellow, black. Yellow, black. Yellow, black. Ooh, black and yellow! Let's shake it up a little. Janet: Barry! Breakfast is ready! Barry: Coming! Hang on a second. (Barry uses his antenna like a phone) Barry: Hello (Through phone) Adam: Barry? Barry: Adam? Adam: Can you believe this is happening? Barry: I can't. I'll pick you up. (Barry flies down the stairs) Martin: Looking sharp. Janet: Use the stairs. Your father paid good money for those. Barry: Sorry. I'm excited.");
}
void fb_plotpixel(int32_t x, int32_t y, uint32_t color){
    size_t fb_index = y * (fb.pitch / 4) + x;
    uint32_t* fb_ptr = (uint32_t*) fb.addr;
    fb_ptr[fb_index] = color;
}
void fb_changebg(uint32_t color){
    ssfn_dst.bg = color;
    for (int y = 0; y < fb.height; y++){
        for(int x = 0; x < fb.width; x++){
            fb_plotpixel(x,y,color);
        }
    }
}
void fb_changefg(uint32_t color){
    ssfn_dst.fg = color;
}
uint32_t fb_getfg(){
    return ssfn_dst.fg;
}
void fb_printchar(char c){
    switch(c){
        case '\n':
            ssfn_dst.x = 0;
            ssfn_dst.y += CHARACTER_HEIGHT;
            break;
        case '\b':
            if(ssfn_dst.x < 6 * CHARACTER_WIDTH) break;
            if(ssfn_dst.x == 0 && ssfn_dst.y == 0) break;
            if(ssfn_dst.x > 0) ssfn_dst.x -= CHARACTER_WIDTH;
            else{
                ssfn_dst.x = fb.width - CHARACTER_WIDTH;
                ssfn_dst.y -= CHARACTER_HEIGHT;
            }
            ssfn_dst.bg = ((uint64_t*)fb.addr)[ssfn_dst.x + ssfn_dst.y * fb.pitch / 4];
            ssfn_putc(' ');
            if(ssfn_dst.x == 0 && ssfn_dst.y == 0) break;
            if(ssfn_dst.x > 0) ssfn_dst.x -= CHARACTER_WIDTH;
            else{
                ssfn_dst.x = fb.width - CHARACTER_WIDTH;
                ssfn_dst.y -= CHARACTER_HEIGHT;
            }
            break;
        default:
            ssfn_putc(c);
            if(ssfn_dst.x >= fb.width){
                ssfn_dst.y += CHARACTER_HEIGHT;
                ssfn_dst.x = 0;
            }
            break;
    }
}
void _putchar(char character){
    fb_printchar(character);
}
void fb_clear(){
    fb_changebg(0x00);
    ssfn_dst.x = 0; ssfn_dst.y = 0;
}
void fb_print(char* str){
    for(uint32_t i = 0; i < strlen(str); i++){
        fb_printchar(str[i]);
    }   
}
void fb_print_color(char* str, uint32_t color){
    uint32_t tmp = ssfn_dst.fg;
    ssfn_dst.fg = color;
    fb_print(str);
    ssfn_dst.fg = tmp;
}

unsigned int *tga_parse(unsigned char *ptr, int size)
{
    unsigned int *data;
    int i, j, k, x, y, w = (ptr[13] << 8) + ptr[12], h = (ptr[15] << 8) + ptr[14], o = (ptr[11] << 8) + ptr[10];
    int m = ((ptr[1]? (ptr[7]>>3)*ptr[5] : 0) + 18);
 
    if(w<1 || h<1) return NULL;
 
    data = (unsigned int*)kmalloc((w*h+2)*sizeof(unsigned int));
    if(!data) return NULL;
 
    switch(ptr[2]) {
        case 1:
            if(ptr[6]!=0 || ptr[4]!=0 || ptr[3]!=0 || (ptr[7]!=24 && ptr[7]!=32)) { kfree(data); return NULL; }
            for(y=i=0; y<h; y++) {
                k = ((!o?h-y-1:y)*w);
                for(x=0; x<w; x++) {
                    j = ptr[m + k++]*(ptr[7]>>3) + 18;
                    data[2 + i++] = ((ptr[7]==32?ptr[j+3]:0xFF) << 24) | (ptr[j+2] << 16) | (ptr[j+1] << 8) | ptr[j];
                }
            }
            break;
        case 2:
            if(ptr[5]!=0 || ptr[6]!=0 || ptr[1]!=0 || (ptr[16]!=24 && ptr[16]!=32)) { kfree(data); return NULL; }
            for(y=i=0; y<h; y++) {
                j = ((!o?h-y-1:y)*w*(ptr[16]>>3));
                for(x=0; x<w; x++) {
                    data[2 + i++] = ((ptr[16]==32?ptr[j+3]:0xFF) << 24) | (ptr[j+2] << 16) | (ptr[j+1] << 8) | ptr[j];
                    j += ptr[16]>>3;
                }
            }
            break;
        case 9:
            if(ptr[6]!=0 || ptr[4]!=0 || ptr[3]!=0 || (ptr[7]!=24 && ptr[7]!=32)) { kfree(data); return NULL; }
            y = i = 0;
            for(x=0; x<w*h && m<size;) {
                k = ptr[m++];
                if(k > 127) {
                    k -= 127; x += k;
                    j = ptr[m++]*(ptr[7]>>3) + 18;
                    while(k--) {
                        if(!(i%w)) { i=((!o?h-y-1:y)*w); y++; }
                        data[2 + i++] = ((ptr[7]==32?ptr[j+3]:0xFF) << 24) | (ptr[j+2] << 16) | (ptr[j+1] << 8) | ptr[j];
                    }
                } else {
                    k++; x += k;
                    while(k--) {
                        j = ptr[m++]*(ptr[7]>>3) + 18;
                        if(!(i%w)) { i=((!o?h-y-1:y)*w); y++; }
                        data[2 + i++] = ((ptr[7]==32?ptr[j+3]:0xFF) << 24) | (ptr[j+2] << 16) | (ptr[j+1] << 8) | ptr[j];
                    }
                }
            }
            break;
        case 10:
            if(ptr[5]!=0 || ptr[6]!=0 || ptr[1]!=0 || (ptr[16]!=24 && ptr[16]!=32)) { kfree(data); return NULL; }
            y = i = 0;
            for(x=0; x<w*h && m<size;) {
                k = ptr[m++];
                if(k > 127) {
                    k -= 127; x += k;
                    while(k--) {
                        if(!(i%w)) { i=((!o?h-y-1:y)*w); y++; }
                        data[2 + i++] = ((ptr[16]==32?ptr[m+3]:0xFF) << 24) | (ptr[m+2] << 16) | (ptr[m+1] << 8) | ptr[m];
                    }
                    m += ptr[16]>>3;
                } else {
                    k++; x += k;
                    while(k--) {
                        if(!(i%w)) { i=((!o?h-y-1:y)*w); y++; }
                        data[2 + i++] = ((ptr[16]==32?ptr[m+3]:0xFF) << 24) | (ptr[m+2] << 16) | (ptr[m+1] << 8) | ptr[m];
                        m += ptr[16]>>3;
                    }
                }
            }
            break;
        default:
            kfree(data); return NULL;
    }
    data[0] = w;
    data[1] = h;
    return data;
}

void display_bmp(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint8_t* ptr, bool clear){
    y = y / fb.pitch;
    dprintf("fb info: h = %d, w = %d\n", fb.height, fb.width);
    uint32_t* bmp = tga_parse(ptr, width*height*3);
    dprintf("image info: h = %d, w = %d\n", bmp[1], bmp[0]);
    uint32_t* fb_ptr = (uint32_t*) fb.addr;
    for(uint32_t i = 0; i < height; i++){
        for(uint32_t j = 0; j < width; j++){
            //if(i == 0 && j < 2) continue;
            uint32_t color = bmp[i * width + j];
            fb_plotpixel(x + j, y + i, color);
        }
    }
    kfree(bmp);
    if(clear){ssfn_dst.x = 0; ssfn_dst.y = 0;}
}