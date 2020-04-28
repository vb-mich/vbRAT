#ifndef VBRAT_VBDEVICEPROPERTIES_H
#endif //VBRAT_VBDEVICEPROPERTIES_H

#define VBRAT_VBDEVICEPROPERTIES_H
#include <sys/system_properties.h>

const char *vbDevProp_GetModel();
typedef struct
{
    char dev_manufacturer[PROP_VALUE_MAX]; //ro.product.brand
    char dev_model[PROP_VALUE_MAX]; //ro.product.name
    char dev_brand[PROP_VALUE_MAX]; //ro.product.vendor.brand
    char dev_modelName[PROP_VALUE_MAX]; //ro.semc.product.name
    char dev_abi[PROP_VALUE_MAX]; //ro.product.cpu.abi
    char gsm_SIMoperator[PROP_VALUE_MAX]; //gsm.sim.operator.alpha
    char gsm_networkType[PROP_VALUE_MAX]; //gsm.network.type
    char gsm_networkOperator[PROP_VALUE_MAX]; //gsm.operator.alpha
    char gsm_operator[PROP_VALUE_MAX]; //gsm.sim.operator.alpha
    char gsm_country[PROP_VALUE_MAX]; //gsm.sim.operator.iso-country
}vbPhonePtoperties_t;
vbPhonePtoperties_t vbDevProp_GetProperties();
void vbDevProp_getActiveNetworks(std::list<std::string> netDevices[2]);
