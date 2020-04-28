#include <dlfcn.h>
#include <sys/system_properties.h>
#include <stdio.h>
#include <cstring>
#include <jni.h>
#include <cstdlib>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <list>
#include <string>
#include <linux/if.h>
#include <tiny_websockets_lib/include/tiny_websockets/network/linux/linux_tcp_client.hpp>
#include <netdb.h>
#include "vbDeviceProperties.h"

// From sdk 21, LoadSystemPropertyGet is not available anymore, whatever, get it from libc
// Snippet from https://github.com/google/cctz/blob/master/src/time_zone_lookup.cc
using property_get_func = int (*)(const char*, char*);
property_get_func LoadSystemPropertyGet()
{
  int flag = RTLD_LAZY | RTLD_GLOBAL;
#if defined(RTLD_NOLOAD)
  flag |= RTLD_NOLOAD;  // libc.so should already be resident
#endif
  if (void* handle = dlopen("libc.so", flag))
  {
    void* sym = dlsym(handle, "__system_property_get");
    dlclose(handle);
    return reinterpret_cast<property_get_func>(sym);
  }
  return nullptr;
}


char cmd_res_line[256];
char total_cmd_res[2048];
char* exec_get_out(char* cmd) {

    FILE* pipe = popen(cmd, "r");

    if (!pipe)
        return NULL;

    total_cmd_res[0] = 0;

    while(!feof(pipe)) {
        if(fgets(cmd_res_line, 256, pipe) != NULL)
        {
            //TODO: add handling for long command reads...
            strcat(total_cmd_res,cmd_res_line);
        }
    }
    pclose(pipe);
    return total_cmd_res;
}

int vb__system_property_get(const char* name, char* value)
{
  static property_get_func system_property_get = LoadSystemPropertyGet();
  return system_property_get ? system_property_get(name, value) : -1;
}

vbPhonePtoperties_t vbDevProp_GetProperties()
{
    vbPhonePtoperties_t ret;
    vb__system_property_get("ro.product.brand", ret.dev_manufacturer);
    vb__system_property_get("ro.product.name", ret.dev_model);
    vb__system_property_get("ro.product.vendor.brand", ret.dev_brand);
    vb__system_property_get("ro.semc.product.name", ret.dev_modelName);
    vb__system_property_get("ro.product.cpu.abi", ret.dev_abi);
    vb__system_property_get("gsm.sim.operator.alpha", ret.gsm_SIMoperator);
    vb__system_property_get("gsm.network.type", ret.gsm_networkType);
    vb__system_property_get("gsm.operator.alpha", ret.gsm_networkOperator);
    vb__system_property_get("gsm.sim.operator.alpha", ret.gsm_operator);
    vb__system_property_get("gsm.sim.operator.iso-country", ret.gsm_country);
    return ret;
}

// https://android.googlesource.com/platform/libcore/+/android-5.1.1_r2/luni/src/main/java/java/net/NetworkInterface.java
void vbDevProp_getActiveNetworks(std::list<std::string> netDevices[2])
{
    struct ifreq ifr;
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd)
    {
        for (int i = 1; if_indextoname(i, ifr.ifr_name) != NULL; i++)
        {
            if (ioctl(fd, SIOCGIFADDR, &ifr) != -1)
            if (ioctl(fd, SIOCGIFFLAGS, &ifr) != -1)
            if (ifr.ifr_ifru.ifru_flags&IFF_UP)
            {
                //if(ifr.ifr_ifru.ifru_flags&IFF_UP)
                {
                    netDevices[0].push_front(ifr.ifr_name);
                    struct sockaddr_in *ipaddr = (struct sockaddr_in *) &ifr.ifr_addr;
                    netDevices[1].push_front(inet_ntoa(ipaddr->sin_addr));
                }
            }

        }
        close(fd);
    }
}

const char *vbDevProp_GetModel()
{
    exec_get_out(
            const_cast<char *>("service call iphonesubinfo 3 | awk -F \"'\" '{print $2}' | sed '1 d' | tr -d '.' | awk '{print}' ORS="));
    char ret[PROP_VALUE_MAX];
    vb__system_property_get("ro.product.model", ret);
    return ret;
}

const char *vbDevProp_GetIMEI()
{
    char ret[PROP_VALUE_MAX];
    vb__system_property_get("ro.product.model", ret);
    return ret;
}