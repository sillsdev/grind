Graphite in InDesign (GrInD)
============================

This is an in-development InDesign 5.5 plug-in designed allow graphite enabled smart fonts to be used in Adobe InDesign. 
This project integrates SIL's Graphite 2 smart font technology with our own implementation of a paragraph composer plugin.
It is currently in early stages of supporting the basic paragraph composition feature set.

Building
--------
Due the way the Adobe InDesign SDK tool DollyXS generates Visual Studio 
projects they are sensitive the placement of the InDesign SDK, which means the 
VS projects files generated on my machine are unlikely to work else where 
without modification. 
This issue in addition to the need to keep certain information confidential 
such as our plugin id prefix means we are unable to publish all the source 
code at this time.
In the future you should be able to build this project provided you have a 
copy of the Adobe InDesign SDK and run their DollyXS tool with the same 
parameters and your own plugin id prefix. 
