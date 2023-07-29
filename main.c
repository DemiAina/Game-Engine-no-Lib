#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include "xdg-shell-protocol.h" 
#include <wayland-client.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

/* This comment is here to explain to my future self what is going on* There are dependencies that are needed for the wayland server (Sway in my case) and the client -> The window im trying to create. 
 * The two files xdg-sell-protocoal -> Headers + Code are used to help the server create the window for the desktop enviroment
* To create them you need to use this : ->
*
* wayland-scaner client-header /user/share.wayland-protocols/stable/xdg-shell/xdg-shell xdg-shell-protocol.h
*
* wayland-scaner private-code /user/share.wayland-protocols/stable/xdg-shell/xdg-shell xdg-shell-protocol.c
*  And this will allow for you some functions that may not exsist
* 
*/


struct wl_compositor *comp;
struct wl_surface *surf;
struct wl_buffer *buf;
struct wl_shm *shared_M;
struct xdg_vm_base *sh;
struct xdg_toplevel *top;
struct wl_seat *seat;
// Keyboard input
struct wl_keyboard *keyboard;

uint8_t *pixel;
uint64_t h = 200;
uint64_t w = 100;
uint8_t c = 0;
uint8_t close_window = 0;

int32_t alloc_shareM(uint64_t sz){
        int8_t name[8];
        name[0] = '/';
        for(uint8_t i = 1; i < 6; i++){
            name[i] = (rand() & 23) + 97;
        }
        int32_t fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, S_IWUSR | S_IRUSR | S_IWOTH | S_IROTH);
        shm_unlink(name);
        ftruncate(fd, sz);
        return fd;
}// Shared memory



void sh_ping(void *data, struct xdg_wm_base *sh, uint32_t ser){
    xdg_wm_base_pong(sh, ser);
}

struct xdg_wm_base_listener sh_list = {
    .ping = sh_ping
};

void keyboard_map(void *data, struct wl_keyboard *keyboard, uint32_t frmt, int32_t fd, uint32_t sz){

}

void keyboard_enter(void *data, struct wl_keyboard *keyboard, uint32_t ser, struct wl_surface *surf, struct wl_array *keys){

}

void keyboard_leave(void *data, struct wl_keyboard *keyboard, uint32_t ser, struct wl_surface *surf){
}

void keyboard_key(void *data , struct wl_keyboard *keyboard, uint32_t ser,uint32_t t, uint32_t key, uint32_t stat){
    // Found out that the OS accutally sends scancodes, to report the position of the keyboard and doesnt send ascii intresting. This is because ascii is the representation of charater in memory
}


void keyboard_mod(void *data, struct wl_keyboard *keyboard, uint32_t ser, uint32_t deps, uint32_t lat, uint32_t lock, uint32_t grp){
}

void keyboard_report(void *data, struct wl_keyboard *keyboard, int32_t rate, int32_t del){
}

struct wl_keyboard_listener keyboard_listen = {
    .keymap = keyboard_map,
    .enter = keyboard_enter,
    .leave = keyboard_leave,
    .key = keyboard_key, 
    .modifiers = keyboard_mod,
    .repeat_info = keyboard_report
};


void seat_capabilities(void *data , struct wl_seat *seat, uint32_t capabilites){

    if(capabilites && WL_SEAT_CAPABILITY_KEYBOARD && !keyboard){
        keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(keyboard, &keyboard_listen, 0);
    }
}

void seat_name(void *data, struct wl_seat *seat, uint8_t *name){

}

struct wl_seat_listener seat_listen = {
        .capabilities = seat_capabilities,
        .name = seat_name
};

void req_glob(void *data, struct wl_registry *reg, uint32_t name, const char *intf, uint32_t v){
    if(!strcmp(intf, wl_compositor_interface.name)){
        comp = wl_registry_bind(reg, name, &wl_compositor_interface, 4);
    }
    else if(!strcmp(intf, wl_shm_interface.name)){
        shared_M = wl_registry_bind(reg, name, &wl_shm_interface, 1);
    }
    else if(!strcmp(intf, xdg_wm_base_interface.name)){
        sh = wl_registry_bind(reg, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(sh, &sh_list, 0);
    }

    else if(!strcmp(intf, wl_seat_interface.name)){
        seat = wl_registry_bind(reg, name, &wl_seat_interface, 1);
        wl_seat_add_listener(seat, &seat_listen, 0);
    }

}

void req_glob_rem(void *data, struct wl_registry *req, uint32_t name){

}

struct wl_registry_listener req_list = {
    .global = req_glob,
    .global_remove = req_glob_rem
};

void resz(){
    int32_t fd = alloc_shareM(w * h * 4);

    pixel = mmap(0, w * h * 4, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    struct wl_shm_pool *pool = wl_shm_create_pool(shared_M, fd, w * h * 4);
    
    buf = wl_shm_pool_create_buffer(pool, 0, w, h ,w * 4, WL_SHM_FORMAT_ARGB8888);
    
    wl_shm_pool_destroy(pool);

    close(fd);
}

void draw(){
    // Set c back to 255 later
    memset(pixel, c, w * h * 4);
    wl_surface_attach(surf, buf, 0 ,0); // Here sway (my compositor will put is where it want it to go
    wl_surface_damage_buffer(surf, 0 , 0 , w, h); // This may cause issues with games need to do research
    wl_surface_commit(surf);
    c++;

}

struct wl_callback_listener cb_listen;

// Refresh frame
void ref_frame(void *data, struct wl_callback *cb , uint32_t callback_data){
    wl_callback_destroy(cb);
    cb = wl_surface_frame(surf);
    wl_callback_add_listener(cb, &cb_listen, 0);
    draw();
}

struct wl_callback_listener cb_listen = {
    .done = ref_frame
};

void xrfc_conf(void *data, struct xdg_surface *xrfc, uint32_t ser){
    xdg_surface_ack_configure(xrfc, ser);
    if(!pixel){
        fprintf(stderr, "Resizing Window\n");
        resz();
    }
    draw();
}

struct xdg_surface_listener xrfc_list = {
    .configure = xrfc_conf
};

void top_conf(void *data, struct  xdg_toplevel *top, int32_t nw, int32_t nh, struct wl_array *stats){
    if (!nw && !nh){
        return;
    }
    if( w != nw || h != nh){
        munmap(pixel, w * h * 4);
        w = nw;
        h = nh;
        resz();
    }
}

void top_close(void *data, struct xdg_toplevel *top){
    close_window = 1;
}

struct xdg_toplevel_listener top_list = {
    .configure = top_conf,
    .close = top_close
};



int main(void){
    struct wl_display *display = wl_display_connect(NULL);
    if(!display){
        fprintf(stderr, "Failed to connect to Wayland display");
        return -1;
    }
    fprintf(stderr, "Connection successful!\n");

    struct wl_registry *reg = wl_display_get_registry(display);
    if(!reg){
        fprintf(stderr, "Failed to get registry\n");
        return -1;
    }

    wl_registry_add_listener(reg, &req_list , 0);
    wl_display_roundtrip(display);

    surf = wl_compositor_create_surface(comp);
    struct wl_callback *callback = wl_surface_frame(surf);
    wl_callback_add_listener(callback, &cb_listen, 0);
    struct xdg_surface *x_surf = xdg_wm_base_get_xdg_surface(sh, surf);
    xdg_surface_add_listener(x_surf, &xrfc_list, 0);

    top = xdg_surface_get_toplevel(x_surf);
    
    xdg_toplevel_add_listener(top, &top_list, 0);
    xdg_toplevel_set_title(top, "Hello World");
    wl_surface_commit(surf);

    while(wl_display_dispatch(display)){
        if(close_window) break; 
    }

    if(keyboard){
        wl_keyboard_destroy(keyboard);
    }

    // Clean up
    wl_seat_release(seat);
    if(buf){
        wl_buffer_destroy(buf);
        fprintf(stderr, "Buffer has been destroyed");
    }
    xdg_toplevel_destroy(top);
    xdg_surface_destroy(x_surf);
    wl_surface_destroy(surf);
    wl_display_disconnect(display);


    return 0;
}
