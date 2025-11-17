# Ban-Rays
**Glasses that detect hidden cameras in other smart glasses**


## Some background

By sending IR at camera lenses, we can take advantage of the fact that the CMOS sensor in cameras reflect light directly back at the source (called 'retro-reflectivity' / 'cat-eye effect') regardless of the angle to identify cameras.

To my dissapointment, this isn't exactly a new idea. Some researchers in 2005 used this property to create 'capture-resistant environments' when smartphones with cameras were gaining popularity. 
* https://homes.cs.washington.edu/~shwetak/papers/cre.pdf

There's even some recent research (2024) that figured out how to classify individual cameras based on their retro-reflections.
* https://opg.optica.org/oe/fulltext.cfm?uri=oe-32-8-13836

Now we have a similar situation on our hands, where smart glasses with hidden cameras seem to be getting pushed by marketing teams. So I want to create a pair of glasses to identify these. Unfortunately, from what I can tell most of the existing research in this space records data with a camera and then uses ML, a ton of controlled angles, etc. to differentiate between normal reflective surfaces and cameras. 

I would feel pretty silly if my solution uses its own camera. So I'll be avoiding that. Instead I think it's likely I'll have to rely on being consistent with my 'sweeps', and creating a good classifier based on signal data. For example you can see here that the back camera on my smartphone seems to produce quick and large spikes, while the glossy screen creates a more prolonged wave.

![](ts_plot_labeled.png)

## Circuit

For prototyping, I'm using:
* Arduino uno
* a bunch of 940nm IR LEDs
* a photodiode as a receiver
* a 2222A transistor

I've found from reading a bunch of stuff that different wavelengths might work better, so I'm on the hunt for those.