## Device Registry
Upon Device detection and removal, the registry is updated to reflect the current system state. <br>
It stores the Devices in a Device Tree plane, among other planes. For example something like this:
```
Root (abstract registry base, platform dependant)
└── USBHostController (managed dynamically)
    └── USBDevice (your USB device)
```
<br>
