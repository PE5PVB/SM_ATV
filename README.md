# SM ATV exciter for 23cm, 13cm or 6cm
23cm, 13cm or 6cm analog ATV exciter

For a while I was looking for a method to easily get on air on 6cm ATV. There are standard transmitters for sale, but those work with channels, have a different video pre-emphasis and are technically not that great either. In addition, it is also nice to build this yourself instead of connecting ready-made modules. But how do you do that? Mix, multiply or directly at the wanted frequency? During my search on AliExpress I came across three VCOs, one of which falls into our 6cm (SM5800) ATV band, the other two, 23cm (SM1200) and 13cm (SM2400). An idea was born. A transmitter, PLL controlled, baseband video in. Many similar circuits already exist, but they were designed quite a long time ago. Many parts are not or only poorly available and actually stop at 13cm. So, to keep it reproducible I have to start from scratch. We live in the 21st century and almost all parts today are SMD. Oops, I hope you keep reading, because I notice that there is a kind of fear of SMD within radio amateurism. Therefore first the following:

What a fantastic time we live in as a self-building radio amateur. There is a huge range of parts available, making a 50 ohm circuit is no longer a problem at all, many parts no longer even need any matching circuit and in the digital domain there is more and more in one chip. Controlling this is often a piece of cake, due to the cheap microcontroller. Okay, there's a downside. All is SMD. But that shouldn't be a problem. The biggest problem for amateurs who don't (want to) work with SMD is fear and ignorance. For those people: Before you throw away any old broken device, take out the PCB and get practicing! I only work with SMD. Why? It is the best soldering method. Part off? No problem, just heat it up and you literally take it off your PCB. And yes, the legs are close together, sometimes they don't even come out from under the part, it is therefore important to use the right tools. A good soldering iron with a good tip. Soldering flux, preferably in the form of gel. A stable and neat workplace with the right light and sometimes some magnification can also be useful. At the moment I am using a JBC soldering station with a hollow tip. In that tip you can put a little bit of tin and you can solder SMD components such as ICs according to the drag technique. Solder flux can be bought in a tube for a few pounds, which will last a long time. Good light and some magnification, take a look at the radio markets, you often buy a beautiful magnifying lamp with built-in LED lighting for less than 50 pounds. And if you have to enlarge it a bit further, for example for soldering QFN parts, a microscope is useful. I use a low-budget microscope for this. These are often sold “for insect viewing”. I therefore take down the argument of not having a steady hand, because I don't have one either. While soldering, my palm is always on the table, eliminating that problem right away. I recently spoke with an amateur friend who is quite old and who I also got into "SMD" a few years ago. He agrees with the above story. It's a matter of practice and more practice. And if you do that with demolition PCB’s, it costs nothing. A few drops of solder at most.

Back to the project. We have the VCO. To make it not too complex, I want to use a PLL with  few parts as possible need to be added. I have been using PLLs from Analog Devices for a while now. This manufacturer has a wide range of PLLs and a very nice development environment. I am referring to the software that is free to download for the evaluation boards. The advantage of the software is that it indicates exactly what must be programmed in which registers for the values you have set. This is very handy when making a prototype, because you then know exactly which bits you have to fire at the part. My eye fell on the ADF4106. This PLL has a built-in prescaler up to 6GHz. The dividers are set via the SPI bus and in addition to a loop filter and a reference signal, only some decoupling capacitors are needed.
The task of the PLL is to get the signal to be supplied in phase, after division, with the reference signal. If you are going to modulate FM then you are actually bullying your PLL, PLL tries to keep it in phase, but your modulator always prevents that. To keep that fight from escalating, add a loop filter. In addition to filtering, you also slow down the correction speed of the PLL so that the effect of the PLL on the final FM signal is as little as possible. The ADF4106 has another nice feature, the MUXOUT. You can configure this pin as a lock detect pin. In other words, the IC gives a signal as soon as the VCO is in phase with the reference signal. This is therefore ideal for controlling your output stage, if you don't, your transmitter will first cycle happily over the band at full power during the start. That lock detector is picky, because if you start making deviation, you may still think that the PLL is locked, but the IC often thinks differently about that. And to have to remove your modulation plug from a transmitter every time you change frequency is not really user-friendly either. It is therefore important not to offer modulation as long as the PLL is not locked. That brings me to the modulator part.

I chose a current-feedback opamp as the input stage. Since almost all components used work on 5V or lower, the opamp must also be able to work on that. It is also useful to be able to turn off the opamp for the aforementioned problem with regard to the PLL lock detect. My eye quickly fell on the OPA695 from Texas Instruments. It doesn't really need to amplify, but it's nice to have a buffer between your input and your VCO. After going through the datasheet I designed the circuit used. The disable pin is controlled by the microcontroller and can enable or disable the opamp. It does not give 100% separation, but more than sufficient for the approval of the lock detector of the PLL. R18 and C20 have been added to compensate for the loss of higher frequencies.

So now we have a VCO, with PLL and an input stage. We give some RF to the PLL to measure and we are left with about 0-3dBm. In other words, 1-2mW. Let's amplify that so we can eventually drive an output stage. We already have 50 ohms, let's keep it that way, then we don't have to match anything. And again, we would like to use 5V or less. Usable up to 6GHz and preferably in two steps, so that we can only switch on that last step once the PLL has given the green light. If you don't do that in two steps, pulling will cause the VCO to sweep over band as soon as you engage the last stage. With this information I started searching and came across the SKY65017 MMIC. It has about 20dB gain and a 1dB compression point of 20dBm. In other words, 100mW. Excellent ability to control an output stage. Since I put two in a row, the signal between the two stages is considerably weakened to prevent overdrive. After some experiments, because the data in the datasheet has of course not been measured on our amateur design with a Chinese FR4 circuit board, I need about 8dB attenuation. This is the Pi attenuator network R21-R22-R23. In practice, it appears that the value of this attenuator sometimes deviates considerably, because there is quite a spread in the output level of the AliExpress VCOs. The last stage is powered by a FET (Q1) which is turned on by the microcontroller.

To control everything, I opted for the well-known Atmega328P. A now somewhat outdated microcontroller, but more than sufficient for controlling this circuit. The advantage is that the software can be written in the widely accepted Arduino IDE. I honestly didn't think it was necessary to include a complete Arduino on a socket in the circuit, so I chose to include only the microcontroller with crystal in the circuit. Programming is easy using a USBASP. A very handy device and can be bought for a few pounds. You connect this to the 6-pin connector J7 and can be used as a programmer via Arduino IDE. Please note, with a blank programmed Atmega328P the “configuration bits” must first be set correctly. The easiest way to do this is to simply program the Arduino bootloader. You only have to do this once. As long as you do not overwrite them, they will remain stored. For the control I chose a rotary encoder and a separate pushbutton, because we like knobs that we can turn and it's also handy that we can "switch off" the transmitter without having to disconnect the power. Since the PLL works at 3.3V and the microcontroller at 5V, a level converter is included. These are Q2, Q3, Q4 and Q5.

The display does not have to be large, because we only want to know which frequency we are tuned to. After some research I came across a cheap 0.91” OLED screen which can be programmed via I2C. Our frequency fits in perfectly here, and is perfectly readable. We put the soldering iron and the PCB away for a while and we grab the old-fashioned paper and pen, because now comes perhaps the most important thing for the user of the transmitter, the user interface.
What do we want, or in this case, what do I want. Change frequency. Since it is a broadband FM signal, you don't have to be accurate to the kHz, but I like a step size of at least 100kHz in the most extreme case. I also want to be able to define the minimum and maximum adjustable frequency, and during testing the display sometimes turned out to be a bit bright, so it would be useful to be able to dim it. So the final menu contains 3 items: contrast (high or low), frequency step size (100kHz, 500kHz or 1MHz), minimum frequency (in 1MHz steps) and maximum frequency (in 1MHz steps). This data, in addition to the tuning frequency and standby status, is stored in the EEPROM of the microcontroller. As soon as the transmitter is put on standby, the tuning frequency is set to a low (unachievable) frequency, in this case 1000MHz causing the VCO voltage to drop to 0 and the output stage is switched off. If I turn the rotary encoder, the frequency can be changed. The display will then start to flash. Only as soon as I press the rotary encoder the new frequency is set, and as soon as the PLL is locked again, the output stage and the modulator are switched on again. I have built the software as simple as possible so that you can adjust it as desired. At the very top of the script you can set the BAND definition to 23, 13 or 6. This way the default values for the desired band are chosen and written to the memory. This makes it easy for people who are not that familiar with the Arduino IDE to program the correct band in the microcontroller while flashing.

The circuit is built on a PCB that fits nicely in a 55x74mm can box. A supply voltage of at least 7V is required for the circuit to work. The higher the voltage, the more energy is converted into heat in the 5V voltage regulator. The circuit does not contain a low-pass filter, so it is unwise to connect it directly to an antenna. All passive parts are SMD size 0603, except C21, D1, R4, R6, and R20, are 1208. C28 is 0805 and the electrolytic capacitors are sized depending on capacity. J4 has been added as a TTL RS232 port. You can connect a USB to RS232 converter to this to connect the circuit to the PC. This part is not yet included in the software, but you can find a challenge here yourself. As a final tip I would like to add that many parts are available through the well-known Chinese web shops. I always recommend ordering extra, despite the fact that the used parts are rarely supplied as counterfeit, unfortunately many components on those websites are rejected production and simply don't work. I have made the gerbers and the Arduino sketch available so that you can have PCB’s made by your PCB manufacturer and expand the software as desired.

Have fun building,
Sjef Verhoeven PE5PVB
