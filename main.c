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
uint8_t *pixel;
uint64_t h = 800;
uint64_t w = 600;

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

    memset(pixel, 255, w * h * 4);
    wl_surface_attach(surf, buf, 0 ,0); // Here sway (my compositor will put is where it want it to go
    wl_surface_damage_buffer(surf, 0 , 0 , w, h); // This may cause issues with games need to do research
    wl_surface_commit(surf);

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
        fprintf(stderr, "Resizing Window");
        resz();
    }
    draw();
}

struct xdg_surface_listener xrfc_list = {
    .configure = xrfc_conf
};

void top_conf(void *data, struct  xdg_toplevel *top, int32_t w, int32_t h, struct wl_array *stats){

}

void top_close(void *data, struct xdg_toplevel *top){
    
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

    }

    // Clean up
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
