#include <iostream>
#include <vector>
#include <memory>

#include <stdlib.h>
#include <libusb.h>

using namespace std;


//10100001   0xA1 bm request type receive
//00100001   send 0x21 

void FailAndExit(std::string message){
	  cout << "error: " << message << endl;
    exit(EXIT_FAILURE);
}

void Handle(int&& code, std::string message){
  if(code !=0)
		FailAndExit(message);
}

static void print_configuration(struct libusb_config_descriptor *config)
{
	int i;

	printf("  Configuration:\n");
	printf("    wTotalLength:         %d\n", config->wTotalLength);
	printf("    bNumInterfaces:       %d\n", config->bNumInterfaces);
	printf("    bConfigurationValue:  %d\n", config->bConfigurationValue);
	printf("    iConfiguration:       %d\n", config->iConfiguration);
	printf("    bmAttributes:         %02xh\n", config->bmAttributes);
	printf("    MaxPower:             %d\n", config->MaxPower);

	//for (i = 0; i < config->bNumInterfaces; i++)
		//print_interface(&config->interface[i]);
}


void HandleUSB(int&& code, std::string message){
	if(code != LIBUSB_SUCCESS){
		switch(code){
			case -3:
			cout << "Access denied (insufficient permissions)" << endl;
		}
		FailAndExit(message);
	}
}

class Configuration {
   private:
      struct libusb_config_descriptor *config;

   public:

	   template <typename USBDevice>
	   Configuration(USBDevice device, int index){
	    HandleUSB(libusb_get_config_descriptor(device, index, &config), "Couldn't retrieve descriptors.\n");
		 }

	   ~Configuration(){
				libusb_free_config_descriptor(config);
		 }

		 void Print(){
			 print_configuration(config);
		 }
};


template <typename USBDevice>
class Device {
	private:
		USBDevice *device = NULL;
		libusb_device_handle *handle = NULL;
   	struct libusb_device_descriptor descriptor;

	public:
		Device(USBDevice *_dev) {
			device = _dev;
			HandleUSB( libusb_get_device_descriptor(device, &descriptor), "getting device descriptor." );
			HandleUSB( libusb_open(device, &handle), "opening usb device.");
		}

	 	string GetManufacturer(){
		  return Describe(descriptor.iManufacturer);
		}

    std::vector<shared_ptr<Configuration>	>
    GetConfigurations(){

			std::vector<shared_ptr<Configuration>	> configs;
    	for (auto i = 0; i < descriptor.bNumConfigurations; i++) {
				auto config = make_shared<Configuration>(device, i);
				config->Print();
				configs.push_back(config);
			}

			return configs;
    }

    int GetPacketSize(){
      return descriptor.bMaxPacketSize0;
    }

		void GetProduct(){
			cout << "Product: " << Describe(descriptor.idProduct) << "\n";
    }

   	template <typename T>
		string Describe(T& descriptor){
			unsigned char str[256];

			libusb_get_string_descriptor_ascii(handle, descriptor, str, sizeof(str));

      return string(reinterpret_cast<char*>(str));
		}

		~Device(){
			libusb_close(handle);
		}
};


class LibUSB {
  private:
    libusb_context *ctx;
    libusb_device **devs = NULL;

  public:
    LibUSB() {
      cout << "starting usb" << endl;
      Handle(libusb_init(&ctx), "starting usb library");
    }


    void GetDevices(){
      auto cnt = libusb_get_device_list(ctx, &devs);

      for(int i=0; i<cnt; i++) {
				auto dev = devs[i];
     		Device<libusb_device> usbdev(dev);

				if(usbdev.GetManufacturer() == "Alienware"){
          cout << "found: " << usbdev.GetManufacturer() << "\n";
          cout << "max packet size: " << usbdev.GetPacketSize() << "\n";
				  usbdev.GetProduct();
					auto cnf = usbdev.GetConfigurations();
				}
      }

    }

    ~LibUSB(){
      cout << "ending usb" << endl;

      if(devs)
			  libusb_free_device_list(devs, 1);

      libusb_exit(ctx);
    }
};

int main(){
  cout << "alien fx keyboard" << endl;

  LibUSB usb;
	usb.GetDevices();
  return 0;
}
