# Music Box

This is a Max object for generating songs using random numbers and simple composition algorithms. It can be given a seed for the random number generation and a tempo as inputs. It generates melody, bass, drums, and piano tracks which need to be formatted into MIDI by Max.

[A video demonstration](https://youtu.be/fyaTH6KR6Uc)

Learn more at [my site](https://cadenkesey.github.io/musicbox.html).

## Using Music Box

Editing this code will require the Max SDK version 8 which can be downloaded from the Cycling '74 website.

[Max SDK](https://cycling74.com/downloads/sdk)

The object outputs notes as integers and floats representing value and length respectively, which means that a few Max objects are needed to join this information into MIDI format.

The Music Algorithm folder is also necessary as it holds all of the patterns for the drums and chord progressions.

## Authors

* **Caden Kesey** - [cadenkesey](https://github.com/cadenkesey)
