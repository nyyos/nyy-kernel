## Device Registry
Upon Device detection and removal, the registry is updated to reflect the current system state. <br>
It stores the Devices in a Device Tree plane, among other planes. For example something like this:
```
Root (abstract registry root, platform independant)
└── PlatformExpert (platform dependant portion, knows the location of buses, enumerates e.g. the ACPI Bus)
    └── USBHostController (managed dynamically)
        └── USBDevice (your USB device)
```
## Driver matching
# Driver Catalogue
Independent from the IO Registry exists the IO Catalogue, storing information about available drivers and loading+probing them on demand. It is responsible for matching Nubs to Drivers etc.

# Future plans
In the (not near) future I want to implement automatic driver matching/loading, possibly inspired by Apple's Info.plist in their KEXTs. Drivers should register to the Catalogue automatically, eventually being unloaded aswell again.
