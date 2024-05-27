#include <usb_names.h>

#define MANUFACTURER_NAME	{'V','i','r','t','u','a','P','i','n'}
#define MANUFACTURER_NAME_LEN	9
#define PRODUCT_NAME	        {'V','i','r','t','u','a','P','i','n',' ','P','i','n','D','M','D','3'}
#define PRODUCT_NAME_LEN	17

struct usb_string_descriptor_struct usb_string_manufacturer_name = {2 + MANUFACTURER_NAME_LEN * 2,3,MANUFACTURER_NAME};
struct usb_string_descriptor_struct usb_string_product_name      = {2 + PRODUCT_NAME_LEN * 2,3,PRODUCT_NAME};
