# OpenGL Sky

This is a 100% procedural sky + clouds renderer. It calculates everything inside a shader.

I can run it without glitches in a cheap 2011 Macbook Air.

*Hello stargazers interested in atmosphere rendering, here's two nice projects you could like: [benanders/Hosek-Wilkie](https://github.com/benanders/Hosek-Wilkie) and [wwwtyro/glsl-atmosphere](https://github.com/wwwtyro/glsl-atmosphere/)*

### What does it look like?

Here's a sample with default settings:

![screenshot](https://user-images.githubusercontent.com/25377830/34966478-1b0da6be-fa43-11e7-97d2-376d92cccc40.png)

Here's one with the "softer" settings.

![softer screenshot](https://user-images.githubusercontent.com/25377830/34967999-ab8331a0-fa4d-11e7-8972-848b924a70e2.png)

### What is it

There are two things here.

#### Blue sky

This thing uses a Mie + Rayleigh scattering function for the sky. Well, almost, it is too heavy so I fitted it to a makeshift math function until it was fast enough.

##### The theory

What is Mie + Rayleigh scattering, do you ask? Well, for our purposes, it's a complex formula used to calculate the color of the sky.

Here's some references:

 - [Wikipedia](https://en.wikipedia.org/wiki/Mie_scattering)
 - [A nice explanation](http://apollo.lsc.vsc.edu/classes/met130/notes/chapter19/rayleigh.html)
 - [Scratchapixel Tutorial](https://www.scratchapixel.com/lessons/procedural-generation-virtual-worlds/simulating-sky/simulating-colors-of-the-sky)
 - [A deep dive into theory with code](https://www.alanzucconi.com/2017/10/10/atmospheric-scattering-3/)
 - [More theory!](http://hyperphysics.phy-astr.gsu.edu/hbase/atmos/blusky.html)
 - [A nifty video](https://www.youtube.com/watch?v=twSg2zbjjnA)

##### The implementation

Here's the mu:

```c
float mu = dot(normalize(pos), normalize(fsun));
```

And here's Rayleigh:

```c
vec3 rayleigh = 3.0 / (8.0 * 3.14) * (1.0 + mu * mu)
```

And Mie:

```c
vec3 mie = (Kr + Km * (1.0 - g * g) / (2.0 + g * g) / pow(1.0 + g * g - 2.0 * g * mu, 1.5)) / (Br + Bm)
```

The extinction is the hard part. I did that on an old Macbook, so I didn't have a lot of processing power. The traditional way of calculating that is using integration. Some imeplementations use 16, 32 or even more steps, and that's VERY HARD for an integrated graphics card.

The thing I did was that I plotted the graph and then came up with a formula that was faster than all the iterations but was still in the ballpark.

So basic curve fitting. 😅😅😅

I don't remember how I came up with the formula! Sorry! :(

```c
vec3 extinction = mix(exp(-exp(-((pos.y + fsun.y * 4.0) * (exp(-pos.y * 16.0) + 0.1) / 80.0) / Br) * (exp(-pos.y * 16.0) + 0.1) * Kr / Br) * exp(-pos.y * exp(-pos.y * 8.0 ) * 4.0) * exp(-pos.y * 2.0) * 4.0, vec3(1.0 - exp(fsun.y)) * 0.2, -fsun.y * 0.2 + 0.5);
```

That's it. Multiply Mie and Rayleigh with the extinction and you have your sky.

```c
color.rgb = rayleigh * mie * extinction;
```

#### Clouds

The clouds use multi-layer brownian noise.

I have a main cloud layer that simulates Cumulus (fluffy) clouds, but it doesn't look that great. You need at least 10 layers of noise for it to look a bit fluffy. I'm using 4, I think. The more you use, the slower it is.

Cumulus: https://upload.wikimedia.org/wikipedia/commons/thumb/3/3c/GoldenMedows.jpg/1200px-GoldenMedows.jpg

There's also a less dense, cloud layer that simulates Cirrus (icy) clouds. This one looks OK and requires a single layer.

Cirrus: https://upload.wikimedia.org/wikipedia/commons/9/94/Cirrus_clouds_mar08.jpg

### Compiling

 - Install `glfw3` (`brew install glfw3` if you're on a Mac).
 - Edit `Makefile` and change the path to `glfw/includes` and `glfw/lib/libglfw3.a` (you can use `libglfw.dylib` here if you don't have `libglfw3.a`).
 - Run `make`. It should work.
 
I didn't put a lot of time into it, so 

### Where are the settings?

They're scattered around the code.

Sorry. Ha!

#### Cirrus Fluffiness ☁️

The `3` below is the amount of cirrus layers you'll render. The more you use, the slower it is 😞

```c
for (int i = 0; i < 3; i++)
```

#### Amount of clouds 🌥

The bigger the number the more there are. From zero to one, although you can go higher. Play around with it.

```
uniform float cirrus = 0.9;
uniform float cumulus = 0.2;
```

#### Speed ⚡️

The `0.2f` below is the speed ratio. Crank it to `5.0f` if you're in a hurry to see the full cycle!

```c
float time = (float)glfwGetTime() * 0.2f - 0.0f;
```

#### Nitrogen Color 🏞

That's not exactly the color of nitrogen, but it is actually related to that.

```c
const vec3 nitrogen = vec3(0.650, 0.570, 0.475);
```

#### Scattering Coefficients 🤓

Play around with those. I got them emphirically, so I can't really explain why they are that amount 🤨

```c
const float Br = 0.0025; // Rayleigh coefficient
const float Bm = 0.0003; // Mie coefficient
const float g =  0.9800; // Mie scattering direction. Should be ALMOST 1.0f
```

Another favourite setting of mine. I think it looks better.

```
const float Br = 0.0020;
const float Bm = 0.0009;
const float g =  0.9200;
```

And another favorite. It is a softer setting.

```c
const float Br = 0.0005;
const float Bm = 0.0003;
const float g =  0.9200;
```
