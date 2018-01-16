# OpenGL Sky

This is a 100% procedural sky + clouds renderer. It calculates everything inside a shader.

I can run it without glitches in a cheap 2011 Macbook Air.

### What does it look like?

Here's a sample with default settings:

![screenshot](https://user-images.githubusercontent.com/25377830/34966478-1b0da6be-fa43-11e7-97d2-376d92cccc40.png)

### What is it

This thing uses a Mie + Rayleigh scattering function for the sky. Well, almost, it is too heavy so I fitted it to a makeshift math function until it was fast enough.

The clouds use multi-layer brownian noise.

I have a main cloud layer that simulates Cumulus clouds, but it doesn't look that great.

Then, there' a less dense, cloud layer that simulates Cirrus (icy) clouds. This one looks OK.

### Compiling

 - Install `glfw3` (`brew install glfw3` if you're on a Mac).
 - Edit `Makefile` and change the path to `glfw/includes` and `glfw/lib/libglfw3.a` (you can use `libglfw.dylib` here if you don't have `libglfw3.a`).
 - Run `make`. It should work.
 
I didn't put a lot of time into it, so 

### Where are the settings?

They're scattered around the code. Sorry

### License?

Please ask me about it and I'll license it to you.
