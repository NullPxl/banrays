# Ban-Rays
**Glasses that detect hidden cameras in other smart glasses**

I'm planning to use 2 main approaches: optics and networking
* With optics, the goal will be to classify all cameras by looking at light reflections.
* With networking, I'll likely be looking for specific, hardcoded identifiers for relevant products in bluetooth packets. 

## Optics

By sending IR at camera lenses, we can take advantage of the fact that the CMOS sensor in a camera reflects light directly back at the source (called 'retro-reflectivity' / 'cat-eye effect') to identify cameras.

To my dissapointment, this isn't exactly a new idea. Some researchers in 2005 used this property to create 'capture-resistant environments' when smartphones with cameras were gaining popularity. 
* https://homes.cs.washington.edu/~shwetak/papers/cre.pdf

There's even some recent research (2024) that figured out how to classify individual cameras based on their retro-reflections.
* https://opg.optica.org/oe/fulltext.cfm?uri=oe-32-8-13836

Now we have a similar situation to those 2005 researchers on our hands, where smart glasses with hidden cameras seem to be getting more popular. So I want to create a pair of glasses to identify these. Unfortunately, from what I can tell most of the existing research in this space records data with a camera and then uses ML, a ton of controlled angles, etc. to differentiate between normal reflective surfaces and cameras. 

I would feel pretty silly if my solution uses its own camera. So I'll be avoiding that. Instead I think it's likely I'll have to rely on being consistent with my 'sweeps', and creating a good classifier based on signal data. For example you can see here that the back camera on my smartphone seems to produce quick and large spikes, while the glossy screen creates a more prolonged wave. 

![](ts_plot_labeled.png)

![](ts_plot_spikes.png)

Right now the spike / camera-like detection is super dependent on the consistensy of 'scans'. If you looks across a reflective object quickly, the slope might be enough to classify as a small camera spike.

### IR Circuit

For prototyping, I'm using:
* Arduino uno
* a bunch of 940nm and 850nm IR LEDs (the 850nm ones appear to reflect much more) 
* a photodiode as a receiver
* a 2222A transistor

![](basicsetup.jpg)

I still need to experiment with how different wavelengths effect results here.

## Networking

This has been more tricky than I first thought! My current approach here is to fingerprint the Meta Raybans over Bluetooth low-energy (BLE) advertisements. But, **I have only been able to detect BLE traffic during 1) pairing 2) powering-on**. The goal is to detect them during usage when they're communicating with the paired phone, but that all seems to be happening over bluetooth classic. And unfortunately the hardware to monitor for ongoing bluetooth classic traffic seems a bit more involved (read: expensive). So I'll likely need a more clever solution here (let me know if you have any).

When turned on or put into pairing mode, I can detect the device through advertised manufacturer data. The `0x01AB` is a Meta-specific SIG-assigned ID (assigned by the Bluetooth standards body).

capture when the glasses are powered on:
```
[01:07:06] RSSI: -59 dBm
Address: XX:XX:XX:XX:XX:XX
Name: Unknown

META/LUXOTTICA DEVICE DETECTED!
  Manufacturer: Meta (0x01AB)
  Service UUID: Meta (0xFD5F) (0000fd5f-0000-1000-8000-00805f9b34fb)

Manufacturer Data:
  Company ID: Meta (0x01AB)
  Data: 020102102716e4

Service UUIDs: ['0000fd5f-0000-1000-8000-00805f9b34fb']
```

IEEE assigns certain MAC address prefixes (OUI, 'Organizationally Unique Identifier'), but these addresses get randomized so I don't expect them to be super useful for BLE.

There are also SIG Assigned Service UUIDs, for examplel 0xFD5F is assigned to Meta. This is probably a proprietary service. Maybe useful.

Here's some links to more data if you're curious:
* https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/Assigned_Numbers/out/en/Assigned_Numbers.pdf
* https://gitlab.com/wireshark/wireshark/-/blob/99df5f588b38cc0964f998a6a292e81c7dcf0800/epan/dissectors/packet-bluetooth.c
* https://www.netify.ai/resources/macs/brands/meta


TODO:
* Read: https://dl.acm.org/doi/10.1145/3548606.3559372
* try active probing/interrogating

---

Thanks to Trevor Seets and Junming Chen for their advice in optics and BLE (respectively). Also to Sohail for lending me meta raybans to test with. 