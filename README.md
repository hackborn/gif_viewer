# gif_viewer
Simple C++ GIF file reader library + example viewer

## compiling
1. Download Cinder 0.9.0 for Visual Studio 2013:<br>
https://libcinder.org/static/releases/cinder_0.9.0_vc2013.zip<br>
(The viewer code contains some Windows-only pieces right now; the library should be Windows-free, although I haven't verified that.)

2. Add an environment variable named CINDER_090_2013 that points to the extracted Cinder folder.

3. And of course, pull and compile this app.

## purpose
There are many great GIF libraries available. The motivation adding my own was primarily pure curiosity about the format, although in looking through various implementations I didn't come across anything that really hit my sweet spot: A couple of image format-independent classes I could drop into a project that had a minimal API for providing my own translation from whatever local bitmap storage class I happen to be using.

So this repo is really two things, an isolated library with a few classes for loading GIF files (and hopefully writing at some point), as well as a simple viewer to verify that everything's working.

## code
The code is in three main pieces:
* Everything in the *src/kt/* folder has been pulled from a generic library for application development I've written.
* Everything in the main *src/* folder is app-specific for the test GIF viewer.
* Everything in *src/gif_io/* handles the actual GIF loading. It's intended that this folder can be dropped in a project to add GIF support. One thing worth noting is that it was written with Visual Studio 2013, and while there shouldn't be anything specific to Visual Studio, you will need at least C++11 compliance.

## using the app
The compiled app is extremely simple. On startup it loads an included sample GIF. If you want to view another GIF, drag and drop onto the app window. There is a single control to change the playback speed. Also, the GIF is automatically scaled to the current window size, so the aspect ratio will be off.

## using the gif_io lib
There are two things to do if you're using the lib in your own code:

1. Create a class that provides storage for and translation to the local bitmap format. In this app, that's the texture_gif_list class.  All it does is subclass the gif::List, a class templatized on the local bitmap format, and supply a std::function in the constructor that receives GIF bitmaps and answers local bitmaps.

2. Create an instance of gif::File and call load() on a file and the gif::List subclasses mentioned above. When load() is finished you'll have a list populated with drawable bitmaps.

## license
There is no license on this code -- do whatever you like -- but I have no responsibility for any errors or damage that results from using it. It's worth noting that the LZW decompression code is based on the very nice code in the Go framework. I'm not sure what impact rewriting it has, but you might want to follow the Go (BSD, I believe) license if you have any concerns.

## contact
Questions or comments, contact me:<br>
Eric Hackborn<br>
hackborn@gmail.com
