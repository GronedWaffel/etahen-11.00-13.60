#include "port_startup_state.hpp"
#include <string>
#include <initializer_list>
#include "private-1240-p5.h"
extern "C" unsigned char private_watch_start[];
extern "C" unsigned char fps_native_start[];
#include "port_firmware.h"
#include "port_toolbox_route.hpp"
#include "port_process_match.hpp"
/* Copyright (C) 2025 etaHEN / LightningMods

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING. If not, see
<http://www.gnu.org/licenses/>.  */


/******************************************************************************
 * Standard and System Header Includes
 ******************************************************************************/
 #include <csignal>
 #include <dirent.h>
 #include <errno.h>
 #include <fcntl.h>
 #include <netinet/in.h>
 #include <pthread.h>
 #include <setjmp.h>
 #include <stdarg.h>
 #include <stdbool.h>
 #include <stdint.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <sys/_iovec.h>
 #include <sys/mount.h>
 #include <sys/signal.h>
 #include <sys/socket.h>
 #include <sys/stat.h>
 #include <sys/sysctl.h>
 #include <sys/types.h>
 #include <sys/un.h>
 #include <sys/wait.h>
 #include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
 
 /******************************************************************************
  * Custom Header Includes
  ******************************************************************************/
 #include <util.hpp>
 #include "port_config.hpp"
 #include "port_stage.hpp"
 #include <freebsd-helper.h>
 
 extern "C" {
 #include "elfldr.h"
 #include "faulthandler.h"
 #include "hbldr.h"
 #include "pt.h"
 #include <ps5/klog.h>
 #include <ps5/kernel.h>

 pid_t elfldr_spawn(const char* cwd, int stdio, uint8_t* elf, const char* name);
 int sceKernelMprotect(void* addr, size_t len, int prot);

 extern uint8_t kstuff_start[];
 extern const unsigned int kstuff_size;
#ifdef ETAHEN_PORT_1360
 extern uint8_t toolbox_card_start[];
#endif

 extern uint8_t fps_prx_start[];
 extern const unsigned int fps_prx_size;

int sceNotificationSend(int userId, bool isLogged, const char* payload);
 }

 
const char json_payload[] =
     "{\n"
     "  \"rawData\": {\n"
     "    \"viewTemplateType\": \"InteractiveToastTemplateB\",\n"
     "    \"channelType\": \"Downloads\",\n"
     "    \"useCaseId\": \"IDC\",\n"
     "    \"toastOverwriteType\": \"No\",\n"
     "    \"isImmediate\": true,\n"
     "    \"priority\": 100,\n"
     "    \"viewData\": {\n"
     "      \"icon\": {\n"
     "        \"type\": \"Url\",\n"
     "        \"parameters\": {\n"
     "          \"url\": \"/user/data/etaHEN/etahen.png\"\n"
     "        }\n"
     "      },\n"
     "      \"message\": {\n"
     "        \"body\": \"etaHEN is starting...\"\n"
     "      },\n"
     "      \"subMessage\": {\n"
     "        \"body\": \"Please Wait For The Welcome Message\"\n"
     "      },\n"
     "      \"actions\": [\n"
     "        {\n"
     "          \"actionName\": \"Go to Debug Settings\",\n"
     "          \"actionType\": \"DeepLink\",\n"
     "          \"defaultFocus\": true,\n"
     "          \"parameters\": {\n"
     "            \"actionUrl\": \"" ETAHEN_TOOLBOX_ROOT_URI "\"\n"
     "          }\n"
     "        }\n"
     "      ]\n"
     "    },\n"
     "    \"platformViews\": {\n"
     "      \"previewDisabled\": {\n"
     "        \"viewData\": {\n"
     "          \"icon\": {\n"
     "            \"type\": \"Predefined\",\n"
     "            \"parameters\": {\n"
     "              \"icon\": \"download\"\n"
     "            }\n"
     "          },\n"
     "          \"message\": {\n"
     "            \"body\": \"etaHEN is starting...\"\n"
     "          }\n"
     "        }\n"
     "      }\n"
     "    }\n"
     "  },\n"
     "  \"createdDateTime\": \"2025-12-14T03:14:51.473Z\",\n"
     "  \"localNotificationId\": \"588193127\"\n"
     "}";
 
 /******************************************************************************
  * Macros and Constants
  ******************************************************************************/
 #define QAFLAGS_SIZE 16
 #define USER_SERVICE_ID 0x80000011
 #define SYSTEM_SERVICE_ID 0x80000010
 #define LNC_UTIL_ERROR_ALREADY_RUNNING 0x8094000c
 #define LNC_ERROR_APP_NOT_FOUND 0x80940031
 #define ENTRYPOINT_OFFSET 0x70
 
 #define PROCESS_LAUNCHED 1
 
 #define LOOB_BUILDER_SIZE 21
 #define LOOP_BUILDER_TARGET_OFFSET 3
 
 #define USLEEP_NID "QcteRwbsnV0"
 
 #define LOOKUP_SYMBOL(resolver, sym) \
   resolver_lookup_symbol(resolver, sym, strlen(sym))
   
 #define SET_FUNCTION_ADDRESS(resolver, function) \
   *(void **)&(function) = \
       (void *)LOOKUP_SYMBOL(resolver, #function) /* NOLINT */
 
 #define BUILD_IOVEC(str) \
   { .iov_base = (str), .iov_length = __builtin_strlen(str) + 1 }
 
 /******************************************************************************
  * Type Definitions and Structures
  ******************************************************************************/
 typedef struct {
   int32_t type;             // 0x00
   int32_t req_id;           // 0x04
   int32_t priority;         // 0x08
   int32_t msg_id;           // 0x0C
   int32_t target_id;        // 0x10
   int32_t user_id;          // 0x14
   int32_t unk1;             // 0x18
   int32_t unk2;             // 0x1C
   int32_t app_id;           // 0x20
   int32_t error_num;        // 0x24
   int32_t unk3;             // 0x28
   char use_icon_image_uri;  // 0x2C
   char message[1024];       // 0x2D
   char uri[1024];           // 0x42D
   char unkstr[1024];        // 0x82D
 } OrbisNotificationRequest; // Size = 0xC30
 
 typedef enum {
   Flag_None = 0,
   SkipLaunchCheck = 1,
   SkipResumeCheck = 1,
   SkipSystemUpdateCheck = 2,
   RebootPatchInstall = 4,
   VRMode = 8,
   NonVRMode = 16,
   Pft = 32UL,
   RaIsConfirmed = 64UL,
   ShellUICheck = 128UL
 } Flag;
 
 typedef struct {
   uint32_t sz;
   int user_id;
   uint32_t app_opt;
   uint64_t crash_report;
   Flag check_flag;
 } LncAppParam;
 
 typedef struct {
   const void *iov_base;
   size_t iov_length;
 } iovec_t;
 
 typedef struct FileDescriptors {
   int fd = 1;
 } FileDescriptor;
 
 typedef struct {
   uint64_t pad0;
   char version_str[0x1C];
   uint32_t version;
   uint64_t pad1;
 } OrbisKernelSwVersion;
 
 typedef struct {
   char prefix[14];  // "etaHEN_PLUGIN" + null terminator
   char titleID[10]; // 4 uppercase letters, 5 numbers, and a null terminator
   char plugin_version[5];
 } CustomPluginHeader;
 
 typedef struct app_info {
   uint32_t app_id;
   uint64_t unknown1;
   uint32_t app_type;
   char     title_id[10];
   char     unknown2[0x3c];
 } app_info_t;
 
 /******************************************************************************
  * External Declarations
  ******************************************************************************/
 extern "C" {
     int sceKernelSendNotificationRequest(int32_t device,
                                          OrbisNotificationRequest *req,
                                          size_t size, int32_t blocking);
     int sceUserServiceGetForegroundUser(uint32_t *userId);
     int sceLncUtilLaunchApp(const char *tid, const char *argv[],
                             LncAppParam *param);
     uint32_t sceLncUtilKillApp(uint32_t appId);
     int sceSystemServiceGetAppId(const char *titleId);
     int sceUserServiceInitialize(void *param);
     int sceKernelGetProsperoSystemSwVersion(OrbisKernelSwVersion *sw);
     int unmount(const char *path, int flags);
     int sceKernelGetAppInfo(int pid, app_info_t *title);
     int sceKernelGetProcessName(int pid, char *name);
     int sceKernelGetOpenPsIdForSystem(void *psid);
     int sceKernelIsGenuineDevKit();

     bool devkit_byepervisor(void);
     void notify(const char *text, ...) {
      OrbisNotificationRequest req;
      va_list args;
    
      memset(&req, 0, sizeof(OrbisNotificationRequest));
    
      // Process args
      va_start(args, text);
      vsnprintf(req.message, sizeof(req.message), text, args);
      va_end(args);
    
      req.type = 0;
      req.unk3 = 0;
      req.use_icon_image_uri = 1;
      req.target_id = -1;
      snprintf(req.uri, sizeof(req.uri), "cxml://psnotification/tex_icon_system");
    
      printf("Notify: %s\n", req.message);
      sceKernelSendNotificationRequest(0, &req, sizeof(req), 0);
    }
    
 }
 
 extern int _write(int fd, const void *, size_t); // NOLINT
 extern ssize_t _read(int, void *, size_t);       // NOLINT
 
 extern const unsigned int daemon_size;
 extern uint8_t daemon_start[];
 extern uint8_t util_start[];
 extern const unsigned int util_size;
 extern uint8_t store_png_start;
 extern const unsigned int store_png_size;
 extern uint8_t sicon_start[];
 extern const unsigned int sicon_size;
 extern uint8_t webman_icon_start[];
 extern const unsigned int webman_icon_size;
 
 /******************************************************************************
  * Global Variables
  ******************************************************************************/
 int plugin_count = 0;
 char buff[255];
 char **loaded_filenames = NULL;
 jmp_buf g_catch_buf;
 FileDescriptor sock;
 
 // Constants
 static const int LOGGER_PORT = 9021;
 static const int STDOUT = 1;
 static const int STDERR = 2;
 
 /******************************************************************************
  * Function Prototypes
  ******************************************************************************/
 void write_embedded_assets();
 bool if_exists(const char *path);
 void notify(const char *text, ...);
static void cleanup(void);
 FileDescriptor FileDescriptor_init(int fd);
 int initStdout();
 void release(FileDescriptor *fd);
 void patch_app_db(void);
 bool is_valid_plugin(const unsigned char *file_buffer);
 uint8_t *get_elf_header_address(unsigned char *file_buffer);
 static bool remount(const char *dev, const char *path);
 
 /******************************************************************************
  * Function Implementations
  ******************************************************************************/
 extern uint8_t shellui_prx_start[];
 extern const unsigned int shellui_prx_size;

  void write_embedded_assets() {
    mkdir("/data/etaHEN/", 0777);
    mkdir("/data/etaHEN/assets/", 0777);
#if 0
    int fd = open("/system_ex/common_ex/lib/shell.prx", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("open failed");
        return;
    }
    if (write(fd, &shellui_prx_start, shellui_prx_size) == -1) {
        perror("write failed");
        return;
    }
    close(fd);
#endif
#if 0
   /// if (!if_exists("/data/etaHEN/fps.prx")) {
        int fd = open("/data/etaHEN/fps.prx", O_WRONLY | O_CREAT | O_TRUNC, 0777);
        if (fd == -1) {
            perror("open failed");
            return;
        }
        if (write(fd, &fps_prx_start, fps_prx_size) == -1) {
            perror("write failed");
        }
        close(fd);
  //  }
#endif

    if (!if_exists("/data/etaHEN/assets/store.png")) {
      int fd = open("/data/etaHEN/assets/store.png", O_WRONLY | O_CREAT | O_TRUNC, 0666);
      if (fd == -1) {
        perror("open failed");
        return;
      }
      if (write(fd, & store_png_start, store_png_size) == -1) {
        perror("write failed");
      }
      close(fd);
    }

    if (!if_exists("/data/etaHEN/assets/webMAN.png")) {
      int fd = open("/data/etaHEN/assets/webMAN.png", O_WRONLY | O_CREAT | O_TRUNC, 0666);
      if (fd == -1) {
        perror("open failed");
        return;
      }
      if (write(fd, & webman_icon_start, webman_icon_size) == -1) {
        perror("write failed");
      }
      close(fd);
    }

    if (!if_exists("/data/etaHEN/etahen.png")) {
      int fd = open("/data/etaHEN/etahen.png", O_WRONLY | O_CREAT | O_TRUNC, 0666);
      if (fd == -1) {
        perror("open failed");
        return;
      }
      if (write(fd, & sicon_start, sicon_size) == -1) {
        perror("write failed");
      }
      close(fd);
    }
 
    if (!if_exists("/system_ex/rnps/apps/NPXS40008/assets/src/modules/categoriesList/assets/texture/etahen_sicon.png")) {
      int fd = open("/system_ex/rnps/apps/NPXS40008/assets/src/modules/categoriesList/assets/texture/etahen_sicon.png", O_WRONLY | O_CREAT | O_TRUNC, 0666);
      if (fd == -1) {
        perror("open failed");
        return;
      }
      if (write(fd, & sicon_start, sicon_size) == -1) {
        perror("write failed");
      }
      close(fd);
    }
 
    if (!if_exists("/mnt/rnps/apps/NPXS40008/assets/src/modules/categoriesList/assets/texture/etahen_sicon.png")) {
      int fd = open("/mnt/rnps/apps/NPXS40008/assets/src/modules/categoriesList/assets/texture/etahen_sicon.png", O_WRONLY | O_CREAT | O_TRUNC, 0666);
      if (fd == -1) {
        perror("open failed");
        return;
      }
      if (write(fd, & sicon_start, sicon_size) == -1) {
        perror("write failed");
      }
      close(fd);
    }
}

  bool is_elf_header(uint8_t* data)
  {
      uint8_t header[] = { 0x7f, 'E', 'L', 'F' };

      return !memcmp(data, header, 4);
  }


  uint8_t* get_kstuff_address(bool& require_cleanup) {
#ifdef ETAHEN_PORT_1360
      // A leftover /data/etaHEN/kstuff.elf may target an older firmware.
      // This build pins the embedded kstuff-lite v1.11 by SHA256.
      require_cleanup = false;
      return kstuff_start;
#else
      const char* path = "/data/etaHEN/kstuff.elf";
      long offset = 0;
      off_t size;
      uint8_t* address;
      int fd;

      if (!if_exists(path)) {
          goto embedded_kstuff;
      }

      fd = open(path, O_RDONLY);
      if (fd <= 0) {
          goto embedded_kstuff;
      }

      size = lseek(fd, 0, SEEK_END);
      address = (uint8_t*)malloc(size);

      if (!address) {
          goto close_fd;
      }

      lseek(fd, 0, SEEK_SET);

      while (offset != size) {
          int n = read(fd, address + offset, size - offset);

          if (n <= 0)
          {
              goto free_mem;
          }

          offset += n;
      }

      if (!is_elf_header(address)) {
          notify( "Kstuff '%s' doesn't have ELF header.", path);
          goto free_mem;
      }

      require_cleanup = true;
      notify("Loading kstuff from: %s", path);
      return address;

  free_mem:
      free(address);
  close_fd:
      close(fd);
  embedded_kstuff:
      require_cleanup = false;
      return kstuff_start;
#endif
  }
 
 bool if_exists(const char *path) {
   struct stat buffer;
   return (stat(path, &buffer) == 0);
 }
 
 static bool remount(const char *dev, const char *path) {
   iovec_t iov[] = {BUILD_IOVEC("fstype"),    BUILD_IOVEC("exfatfs"),
                    BUILD_IOVEC("fspath"),    BUILD_IOVEC(path),
                    BUILD_IOVEC("from"),      BUILD_IOVEC(dev),
                    BUILD_IOVEC("large"),     BUILD_IOVEC("yes"),
                    BUILD_IOVEC("timezone"),  BUILD_IOVEC("static"),
                    BUILD_IOVEC("async"),     {NULL, 0},
                    BUILD_IOVEC("ignoreacl"), {NULL, 0}};
   return nmount((struct iovec *)iov, sizeof(iov) / sizeof(iov[0]),
                 MNT_UPDATE) == 0;
 }
 static void cleanup(void) { 
    if (sock.fd != -1) {
      close(sock.fd);
      sock.fd = -1;
    }
  
    // Notify user about cleanup
    notify("etaHEN has been cleaned up.");
  
    // Exit the program
    exit(0);
 }
 
 // FileDescriptor methods implementations
 FileDescriptor FileDescriptor_init(int fd) {
   FileDescriptor newFd;
   newFd.fd = fd;
   return newFd;
 }
 
 void release(FileDescriptor *fd) { 
   fd->fd = -1; 
 }
 
 // Stdout initialization logic
 int initStdout() {
   // Check for logging file existence logic here
   // For simplicity, I'm assuming it always exists
   char error_msg[500] = {0};
 
   sock.fd = -1;
   sock = FileDescriptor_init(socket(AF_INET, SOCK_STREAM, 0));
   if (sock.fd == -1) {
     snprintf(error_msg, sizeof(error_msg), "Failed to create socket: %s",
              strerror(errno));
     notify(error_msg);
     return -1;
   }
 
   int value = 1;
   if (setsockopt(sock.fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) < 0) {
     snprintf(error_msg, sizeof(error_msg), "Failed to set socket options: %s",
              strerror(errno));
     notify(error_msg);
     return -1;
   }
 
   struct sockaddr_in server_addr;
   (void)memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   server_addr.sin_port = htons(LOGGER_PORT);
   server_addr.sin_addr.s_addr = 0;
 
   if (bind(sock.fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
     snprintf(error_msg, sizeof(error_msg), "Failed to bind socket: %s",
              strerror(errno));
     notify(error_msg);
     return -1;
   }
 
   if (listen(sock.fd, 1) != 0) {
     snprintf(error_msg, sizeof(error_msg), "Failed to listen on socket: %s",
              strerror(errno));
     notify(error_msg);
     return -1;
   }
 
   struct sockaddr client_addr;
   socklen_t addr_len = sizeof(client_addr);
   int conn = accept(sock.fd, &client_addr, &addr_len);
   if (conn != -1) {
     dup2(conn, STDOUT);
     dup2(conn, STDERR);
     close(conn);
     return conn;
   }
 
   snprintf(error_msg, sizeof(error_msg), "Failed to accept connection: %s",
            strerror(errno));
   notify(error_msg);
   return -1;
 }
 
 // Function to check if the file buffer contains a valid custom plugin header
 bool is_valid_plugin(const unsigned char *file_buffer) {
   // Check if the prefix matches
   if (strncmp((const char *)file_buffer, "etaHEN_PLUGIN", 13) != 0) {
     puts("Plugin header prefix does not match");
     return false;
   }
 
   // Validate the title ID format (4 uppercase letters followed by 4 numbers)
   const CustomPluginHeader *header = (const CustomPluginHeader *)file_buffer;
   for (int i = 0; i < 4; ++i) {
     if (header->titleID[i] < 'A' || header->titleID[i] > 'Z') {
       puts("Invalid plugin file: titleID must contain 4 uppercase letters as "
            "the start");
       return false;
     }
   }
   for (int i = 4; i < 9; ++i) {
     if (header->titleID[i] < '0' || header->titleID[i] > '9') {
       puts("Invalid plugin file: titleID must contain 5 numbers as the end");
       return false;
     }
   }
 
   // Ensure the title ID is null-terminated
   if (header->titleID[9] != '\0') {
     puts("Invalid plugin file: titleID must be null-terminated");
     return false;
   }
 
   for (int i = 0; i < 3; ++i) {
     if (header->plugin_version[i] == '.') {
       continue;
     } else if (header->plugin_version[i] < '0' ||
                header->plugin_version[i] > '9') {
       puts(
           "Invalid plugin file: version must be in the following format xx.xx");
       return false;
     }
   }
 
   return true;
 }
 
 // Function to return the address of the ELF header, skipping the custom plugin header
 uint8_t *get_elf_header_address(unsigned char *file_buffer) {
   // The ELF header should start right after the custom plugin header
   return file_buffer + sizeof(CustomPluginHeader);
 }
 

pid_t find_pid(const char * name) {
  int mib[4] = {
    CTL_KERN,
    KERN_PROC,
    KERN_PROC_PROC,
    0
  };
  size_t buf_size;
  void * buf;

  int pid = -1;
  // determine size of query response
  if (sysctl(mib, 4, NULL,&buf_size, NULL, 0)) {
    printf("sysctl failed: %s\n", strerror(errno));
    return -1;
  }

  // allocate memory for query response
  if (!(buf = malloc(buf_size))) {
    printf("malloc failed %s\n", strerror(errno));
    return -1;
  }

  // query the kernel for proc info
  if (sysctl(mib, 4, buf,&buf_size, NULL, 0)) {
    printf("sysctl failed: %s\n", strerror(errno));
    free(buf);
    return -1;
  }

  for (char * ptr = static_cast < char * > (buf); ptr < (static_cast < char * > (buf) + buf_size);) {
    struct kinfo_proc * ki = reinterpret_cast < struct kinfo_proc * > (ptr);
    ptr += ki->ki_structsize;

    if (port_other_process_matches(ki->ki_pid, getpid(), ki->ki_comm,
                                   sizeof(ki->ki_comm), name)) {
      pid = ki->ki_pid;
      break;
    }
  }

  free(buf);

  return pid;
}

bool is_elf_file(const void* buffer, size_t size) {
    if (size < 4) return false;
    
    const unsigned char elf_magic[] = {0x7F, 'E', 'L', 'F'};
    return memcmp(buffer, elf_magic, 4) == 0;
}


#include "../../include/port_plugin_runtime.h"
bool load_plugin(const char *path, const char *filename)
{
  (void)filename;
  return port_plugin_load(path, sock.fd, sceKernelGetProcessName, elfldr_spawn) != 0;
}

/*=================== LOAD PLUGINS =========================*/
char **find_plugin_files() {
  plugin_count=0;loaded_filenames=nullptr;
  char **paths=nullptr;
  for(const char *base : {"/mnt/usb0","/mnt/usb1","/mnt/usb2","/mnt/usb3","/mnt/usb4","/mnt/usb5","/mnt/usb6","/mnt/usb7","/data"})
    for(const char *name : {"etaHEN","etahen"})
      for(const char *kind : {"plugins","payloads"}){
        std::string directory=std::string(base)+"/"+name+"/"+kind;
        DIR *dir=opendir(directory.c_str());if(!dir)continue;
        dirent *entry;
        while((entry=readdir(dir))){
          if(!port_plugin_suffix(entry->d_name,".elf") && !port_plugin_suffix(entry->d_name,".plugin"))continue;
          std::string path=directory+"/"+entry->d_name;
          if(access((path+".auto_start").c_str(),F_OK))continue;
          struct stat st{};
          if(lstat(path.c_str(),&st) || !S_ISREG(st.st_mode))continue;
          bool duplicate=false;
          for(int i=0;i<plugin_count;i++)if(path==paths[i])duplicate=true;
          if(duplicate)continue;
          char *saved_path=strdup(path.c_str()), *saved_name=strdup(entry->d_name);
          if(!saved_path || !saved_name){free(saved_path);free(saved_name);continue;}
          char **next=(char**)realloc(paths,(plugin_count+1)*sizeof(char*));
          if(!next){free(saved_path);free(saved_name);continue;}
          paths=next;
          next=(char**)realloc(loaded_filenames,(plugin_count+1)*sizeof(char*));
          if(!next){free(saved_path);free(saved_name);continue;}
          loaded_filenames=next;paths[plugin_count]=saved_path;loaded_filenames[plugin_count]=saved_name;
          plugin_count++;
        }
        closedir(dir);
      }
  return paths;
}
void free_plugin_files(char **paths) {
  for(int i=0;i<plugin_count;i++){free(paths[i]);free(loaded_filenames[i]);}
  free(paths);free(loaded_filenames);loaded_filenames=nullptr;plugin_count=0;
}

bool Byepervisor();
bool sceKernelIsTestKit() {
  uint8_t s_PsId[16] = {0};

  size_t v2 = 16;
  if (sysctlbyname("machdep.openpsid_for_sys", &s_PsId, &v2, 0, 0) < 0) {
    printf("sceKernelGetOpenPsIdForSystem failed\n");
    return true;
  }

  char psid_buf[255] = {0};

  for (int i = 0; i < 16; i++) {
    snprintf(psid_buf + strlen(psid_buf), 255 - strlen(psid_buf), "%02x",
             s_PsId[i]);
  }

  const char *whitelisted_psids[] = {
      "b345df7d4c77618d40f19a90e438ad87",
      "ab535275b7196e7e7d43f4f9e7806724",
      "d376c7780b960e5182d326ba3aa2d7a3",
      "a8d89ad976b5cb912837ad29b0cc4610",
      "177e09480b40816a1caca5151565daa5",
           

  };

#if 0
  printf("PSID: %s\n", psid_buf);
  char buff[300];
  snprintf(buff, sizeof buff, "PSID: %s", psid_buf);
  notify(buff);
#endif

  for (int i = 0; i < sizeof(whitelisted_psids) / sizeof(whitelisted_psids[0]);
       i++) {
    if (strcmp(psid_buf, whitelisted_psids[i]) == 0) {
      // printf("PSID (%s) whitelisted\n", psid_buf);
      return false; // report not testkit if is whitelisted
    }
  }

  // printf("PSID (%s) Not whitelisted\n", psid_buf);
  return if_exists("/system/priv/lib/libSceDeci5Ttyp.sprx");
}
#define PUBLIC_TEST 0
#ifdef ETAHEN_PORT_1360
#include "port_kstuff.hpp"
#endif
#define EXPIRE_YEAR 2026
#define EXPIRE_MONTH 1
#define EXPIRE_DAY 1


bool isPastBetaDate(int year, int month, int day);

int main(void) {
  port_stage("bootstrap","RUN BEGIN; entered main");
  port_result("bootstrap","firmware raw",kernel_get_fw_version(),0);
#ifdef ETAHEN_PORT_1360
  if (!snipers_firmware_profile(kernel_get_fw_version())) {
    port_result("bootstrap","firmware profile unsupported",-1,0);
    notify("Unsupported firmware: this etaHEN build requires a supported 11.00-13.60 profile");
    return 1;
  }
  const auto* diagnostic_profile=snipers_firmware_profile(kernel_get_fw_version());
  char profile_detail[512];
  snprintf(profile_detail,sizeof profile_detail,"profile=%s allproc=%lx security=%lx rootvnode=%lx textDelta=%lx sysentvec=%lx sysentvecPs4=%lx crypt=%lx sysents=%lx sysentsPs4=%lx pager=%lx kstuff=v1.11",
    diagnostic_profile->name,diagnostic_profile->allproc,diagnostic_profile->security,diagnostic_profile->rootvnode,diagnostic_profile->textDelta,diagnostic_profile->sysentvec,diagnostic_profile->sysentvecPs4,diagnostic_profile->cryptSingletonArray,diagnostic_profile->sysents,diagnostic_profile->sysentsPs4,diagnostic_profile->pagerTable);
  port_stage("bootstrap",profile_detail);
  const pid_t existing_etahen = find_pid("etaHEN");
  if (existing_etahen > 0) {
    char stage[96];
    snprintf(stage, sizeof(stage), "resident etaHEN detected: other pid=%d", existing_etahen);
    port_stage("bootstrap", stage);
    notify("etaHEN is already running. Restart before changing builds.");
    return 1;
  }
#endif
  // ptrace(PT_ATTACH, pid, 0, 0);
  /// clearFramePointer();
  int pid = -1;

#if BETA == 1
  char out[1024];
#endif

  signal(SIGCHLD, SIG_IGN);

  klog_puts("Jailbreaking the boostrapper ...");
  // launch socksrv.elf in a new processes
  port_stage("bootstrap","before privilege setup");
  int privilege_result=elfldr_raise_privileges(getpid());
  port_result("bootstrap","privilege setup",privilege_result,privilege_result?errno:0);
  if (privilege_result) {
    notify("Unable to raise privileges");
    return -1;
  }
#ifdef ETAHEN_PORT_1360
  // Clear before spawning the critical daemon, even if its PID is reused.
  unlink("/system_tmp/etahen-experimental-startup");
  unlink("/system_tmp/etahen-experimental-startup.tmp");
#endif

#if BETA == 1
  printf("Get_code %d", GetDecryptedConsoleCode(
                            &out[0])); // ignore return value because we need to
                                       // call is_console_whitelisted anyway
  bool is_whitelisted = is_console_whitelisted(
      &buffer[0], &out[0]); // gets PSID if its not whitelisted too
#endif

#if BETA == 1 || PUBLIC_TEST == 1
  if (isPastBetaDate(EXPIRE_YEAR, EXPIRE_MONTH, EXPIRE_DAY)) {
    notify("This etaHEN Beta version expired on %d-%d-%d", EXPIRE_YEAR,
           EXPIRE_MONTH, EXPIRE_DAY);
    return -1;
    raise(SIGSEGV);
  }
#endif

#if 0
  if (sceKernelIsTestKit()) {
    notify("support dropped for testkits if you donated to my ko-fi and are NOT andrew send me a message");
    return 0;
  }
#endif


  klog_printf("   Success!\n");
  if(if_exists("/data/I_want_logging_for_etahen")){
      klog_printf("Redirecting stdout and stderr to logger ...");
     if(initStdout() >= 0)
         klog_puts("   Success!");
     else
         klog_puts("   Failed!");
      
  }


  
  #if BETA == 1 
  if (!is_whitelisted) {
    notify("This console is NOT approved to use this etaHEN beta version\n\nIf "
           "you are not yet approved send LM the pending_approval.bin file "
           "from your USB for the etaHEN_approval.bin");
    int fd = open("/mnt/usb0/pending_approval.bin", O_CREAT | O_TRUNC | O_RDWR,
                  0777);
    if (fd < 0) {
      fd = open("/mnt/usb1/pending_approval.bin", O_CREAT | O_TRUNC | O_RDWR,
                0777);
      if (fd < 0) {
        fd = open("/mnt/usb2/pending_approval.bin", O_CREAT | O_TRUNC | O_RDWR,
                  0777);
      }

    if (fd >= 0) {
      write(fd, buffer, strlen(buffer));
      close(fd);
    } else {
      notify("No USB Found to save pending_approval.bin\n\nInsert a EXFAT USB "
             "then re-run this payload");
    }

    return -1;
    raise(SIGSEGV);
  }
  #endif


  OrbisKernelSwVersion sys_ver;
  sceKernelGetProsperoSystemSwVersion(&sys_ver);

#ifndef ETAHEN_PORT_1360
  if (sys_ver.version < 0x3000000 && !sceKernelIsGenuineDevKit()) {
    klog_printf("FW %s version has Byepervisor available, sstarting....\n", sys_ver.version_str);
    if (!Byepervisor()) {
      printf("Byepervisor failed or is resume_nedded");
      return 0;
    }
  }

  klog_puts("============== Spawner (Bootstrapper) Started =================");
#else
  klog_puts("Unofficial etaHEN 2.5B multi-firmware community test");
#endif

  mkdir("/data/etaHEN", 0777);
  mkdir("/data/etaHEN/plugins", 0777);
  mkdir("/data/etaHEN/payloads", 0777);
  mkdir("/data/etaHEN/daemons", 0777);
  mkdir("/data/etaHEN/assets", 0777);
  mkdir("/data/etaHEN/games", 0777);
#ifdef ETAHEN_PORT_1360
  // Keep other etaHEN releases' configuration formats intact.
  int config_fd=open(ETAHEN_CONFIG_PATH,O_WRONLY|O_CREAT|O_EXCL,0666);
  if(config_fd>=0){
    const char defaults[]="[Settings]\nPS5Debug=0\nFTP=1\ntoolbox_auto_start=1\nDPI=0\nDPI_v2=0\nKlog=0\nAllow_data_in_sandbox=1\noverlay_ram=0\noverlay_cpu=0\noverlay_gpu=0\noverlay_fps=0\n";
    bool written=write(config_fd,defaults,sizeof(defaults)-1)==sizeof(defaults)-1;close(config_fd);
    if(!written){unlink(ETAHEN_CONFIG_PATH);notify("Unable to write the port configuration");return -1;}
  }else if(errno!=EEXIST){notify("Unable to open the port configuration");return -1;}
  rename("/data/etaHEN/bootstrap-experimental.log","/data/etaHEN/bootstrap-experimental.previous.log");
  freopen("/data/etaHEN/bootstrap-experimental.log","w",stdout);setvbuf(stdout,nullptr,_IONBF,0);
#endif

  klog_printf("Registering signal handler ...");
  fault_handler_init(cleanup);
  klog_printf("   Success!\n");

  klog_printf("Remounting system partitions ...");
#ifdef ETAHEN_PORT_1360
  puts("Preparing etaHEN system assets");
#endif
  port_stage("bootstrap","before system remounts");
  if (!remount("/dev/ssd0.system_ex", "/system_ex")) {
    port_result("bootstrap","system_ex remount failed",-1,errno);
    perror("failed to mount /system_ex\nif you see this reboot");
    notify("failed to mount /system_ex\nif you see this reboot");
    return -1;
  }
  if (!remount("/dev/ssd0.system", "/system")) {
    port_result("bootstrap","system remount failed",-1,errno);
    perror("failed to mount /system_\nif you see this reboot");
    notify("failed to mount /system\nif you see this reboot");
    return -1;
  }
  klog_printf("   Success!\n");

  klog_printf("Writing embedded assets ...");
  port_stage("bootstrap","before writing assets");
  write_embedded_assets();
  port_stage("bootstrap","assets written");
  klog_printf("   Written!\n");

	sceNotificationSend(0xFE, true, &json_payload[0]);
  port_stage("bootstrap","starting notification sent; before update unmount");

  klog_printf("Unmounting /update forcefully ...");
  // block updates
  unlink("/update/PS5UPDATE.PUP");
  unlink("/update/PS5UPDATE.PUP.net.temp");
  // unlink("/update/PS4UPDATE.PUP.md5");
  if ((int)unmount("/update", 0x80000LL) < 0) {
    unmount("/update", 0);
  }

  klog_puts("   Success!");

#if 1
  port_stage("bootstrap","update unmount finished; before kstuff inspection");
  char buz[100] = { 0 };
  // Load kstuff if needed
  bool dont_load_kstuff = (if_exists("/mnt/usb0/no_kstuff") || if_exists("/data/etaHEN/no_kstuff"));
#ifdef ETAHEN_PORT_1360
  const auto kstuff_snapshot=port_read_kstuff_state();
  const auto kstuff_state=kstuff_snapshot.classify(KERNEL_ADDRESS_DATA_BASE);
  char state_detail[192];snprintf(state_detail,sizeof(state_detail),"kstuff state=%d readable=%d native=%016lx compat=%016lx crypt=%04x/%04x",(int)kstuff_state,kstuff_snapshot.readable,kstuff_snapshot.native,kstuff_snapshot.compat,kstuff_snapshot.xts,kstuff_snapshot.hmac);
  port_stage("bootstrap",state_detail);
  const bool kstuff_running = find_pid("kstuff") > 0 || kstuff_state==PortKstuffState::Installed;
  if(!dont_load_kstuff&&!kstuff_running&&kstuff_state==PortKstuffState::Unknown){
      notify("etaHEN stopped: existing kstuff state is inconclusive; no duplicate payload loaded");
      port_stage("bootstrap","stopped before kstuff load: unknown kernel hook state");
      return 1;
  }
#else
  const bool kstuff_running = false;
#endif
  port_stage("bootstrap",kstuff_running?"resident kstuff found":"no resident kstuff found");
  if (dont_load_kstuff) {
      notify("kstuff loading disabled via file, non-payload homebrew and PS4 FPKGs will be disabled");
      klog_puts("kstuff loading disabled in config.ini or no_kstuff file found");
  }
  if (!dont_load_kstuff && !kstuff_running && sys_ver.version >= 0x3000000) {
      port_stage("bootstrap","before spawning kstuff");
      //notify("Loading kstuff ...");

      bool cleanup_kstuff = false;
      uint8_t* kstuff_address = get_kstuff_address(cleanup_kstuff);

      int kstuff_pid=elfldr_spawn("/", STDOUT_FILENO, kstuff_address, "kstuff");
      port_result("bootstrap","kstuff spawn pid",kstuff_pid,kstuff_pid<0?errno:0);
      if (kstuff_pid >= 0) {
          int wait = 0;
          bool kstuff_not_loaded = false;
          sleep(1);
          while ((kstuff_not_loaded = sceKernelMprotect(&buz[0], 100, 0x7) < 0)) {
              if (wait++ > 10) {
                  notify("Failed to load kstuff, kstuff will be unavailable");
                  break;
              }
              sleep(1);
          }

          port_result("bootstrap","kstuff readiness",kstuff_not_loaded?-1:0,0);
          port_result("bootstrap","kstuff wait seconds",wait,0);
          if (!kstuff_not_loaded)
              klog_puts("kstuff loaded");

          if (cleanup_kstuff) {
              free(kstuff_address);
          }
      }
      else {
          notify("Failed to load kstuff, kstuff will be unavailable");
      }
  }
  sleep(1);
#endif

  klog_printf("Starting Utility etaHEN services ...");
  port_stage("bootstrap","before utility spawn");
#ifdef ETAHEN_PORT_1360
  puts("Starting etaHEN utility service");
#endif

  while ((pid = find_pid("etaHEN")) > 0) {
   // printf("killing pid %d\n", pid);
    if (kill(pid, SIGKILL)) {
      perror("kill");
    }
  }

  if (elfldr_spawn("/", sock.fd, util_start, "etaHEN Utility Daemon") >= 0) {
      port_stage("bootstrap","utility spawned");
      klog_printf("  Launched!\n");
    // Open the file with write permission, create if not exist, truncate to zero if exists
    int fd = open("/data/etaHEN/daemons/util.elf", O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (fd == -1) {
      perror("open failed");
      return -1337;
    }
    // Write the buffer to the file
    if (write(fd, util_start, util_size) == -1) {
       perror("write failed");
    }

    // Close the file descriptor
    close(fd);
  } else {
    klog_printf("failed to launch utility daemon\n");
    notify("failed to launch the etaHEN utility daemon");
    return -2;
  }

  unlink(P5_TARGET_PATH);unlink(P5_TARGET_PATH ".tmp");
  int observer_pid=elfldr_spawn("/",sock.fd,private_watch_start,"Snipers Controller P6 Watch");
  port_result("bootstrap","p5 observer spawn",observer_pid,observer_pid<0?errno:0);
  klog_printf("Starting the main etaHEN daemon ...");
  port_stage("bootstrap","before critical-service spawn");
#ifdef ETAHEN_PORT_1360
  puts("Starting etaHEN critical service");
#endif

  int critical_pid=elfldr_spawn("/",sock.fd,daemon_start,"etaHEN Critical services");
  if (critical_pid >= 0) {
      port_stage("bootstrap","critical service spawned");
      klog_printf("  Launched!\n");
  } else {
      klog_printf("failed to launch main daemon\n");
      notify("failed to launch the main etaHEN daemon");
      return -2;
  }

#ifdef ETAHEN_PORT_1360
  if (snipers_firmware_profile(kernel_get_fw_version())) {
  // The embedded helper owns only ETHN13600. It skips the registration scan
  // when its existing receipt, metadata and artwork already match.
  port_stage("bootstrap","before Toolbox card helper spawn");
  if(elfldr_spawn("/",sock.fd,toolbox_card_start,"etaHEN Card Setup")<0){
    port_stage("bootstrap","Toolbox card helper spawn failed");
    notify("etaHEN is running, but the Toolbox card installer could not start");
  }else port_stage("bootstrap","Toolbox card helper spawned; result is toolbox-card-install.json");
  }
#endif

  // return 0;

  if(elfldr_spawn("/",sock.fd,fps_native_start,"etaHEN FPS Sampler")<0)
    port_stage("bootstrap","native FPS sampler could not start");
  port_stage("bootstrap","before plugin discovery");
  char **plugin_paths = find_plugin_files();
  port_result("bootstrap","plugin count",plugin_count,0);
  bool plugins_ready=false;
  if(plugin_count>0){
    for(int tick=0;tick<600;tick++){
      FILE *f=fopen(PORT_STARTUP_PATH,"rb");PortStartupRecord record;
      bool valid=port_read_startup(f,record);if(f)fclose(f);
      if(valid){int state=record.evaluate(critical_pid,find_pid("SceShellUI"));
        if(state==1){plugins_ready=true;break;}if(state<0)break;}
      usleep(100000);
    }
    if(!plugins_ready)notify("Plugin auto-start skipped: etaHEN startup did not confirm readiness");
  }
  if (plugin_paths && plugin_count > 0 && plugins_ready) {
    int loaded_plugins = 0;
    // First, load all plugins except elfldr.plugin
    for (int i = 0; i < plugin_count; i++) {
      // Skip loading elfldr.plugin in this loop
      if (strcmp(loaded_filenames[i], "elfldr.plugin") != 0) {
          klog_printf("Loading plugin: %s\n", plugin_paths[i]);
        if (!load_plugin(plugin_paths[i], loaded_filenames[i])) {
          snprintf(buff, sizeof(buff),
                   "[etaHEN] Failed to load plugin!\nPath: %s",
                   plugin_paths[i]);
          notify(buff);
          klog_puts("FAILED!");
          continue;
        }

        klog_puts("Loaded!");
        loaded_plugins++;
      }
    }
    //(void)memset(buff, 0, sizeof(buff));
    // snprintf(buff, sizeof(buff), "Successfully loaded %d plugins",
    // loaded_plugins); notify(buff);
    klog_printf("Successfully loaded %d plugins\n", loaded_plugins);
  }
  free_plugin_files(plugin_paths);
  // raise(SIGKILL, getpid());
  // sceSystemServiceLoadExec("exit", NULL);
  klog_puts("============== Spawner (Bootstrapper) Finished =================");
#ifdef ETAHEN_PORT_1360
  port_stage("bootstrap","RUN bootstrap complete; services dispatched, readiness is separate");
  puts("Both etaHEN services spawned; check Toolbox initialization separately");
#endif

  return 0;
}
