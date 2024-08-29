# Introduction

##Brief overview of Arduino timers

Greetings, this post is sort of a self-teaching tool and also a response to u/3Domese3, but in this post, we're going to experiment with Arduino timers. Forget about those cozy libraries - we're going straight to the metal!
First things first, let's identify the key players in our timing game. The most common timer and the one we'll focus the most on is Timer1, which is a 16-bit timer available on most Arduino boards.

##Importance of direct timer manipulation

While Arduino libraries are great for beginners, diving into direct timer manipulation opens up a world of precision and efficiency. It's like switching from an automatic transmission to a manual - you gain complete control over your timing operations, allowing for more complex and optimized applications.

#Timer Registers and Their Addresses

These registers are the control panel for our timer operations. By manipulating them directly, we're essentially speaking the microcontroller's native language, allowing for faster execution and finer control than high-level Arduino functions could ever provide.

The main registers we'll be invoking are:

TCCR1A and TCCR1B - Timer/Counter Control Registers
TCNT1 - Timer/Counter Register
OCR1A and OCR1B - Output Compare Registers
ICR1 - Input Capture Register
TIMSK1 - Timer Interrupt Mask Register
TIFR1 - Timer Interrupt Flag Register

You can find the references to these in the header files, but to save you the trouble searching they map directly to 


    //      Timer    Register Address
	#define TCCR1A  _SFR_MEM8(0x80)
	#define TCCR1B  _SFR_MEM8(0x81)
	#define TCNT1   _SFR_MEM16(0x84)
	#define OCR1A   _SFR_MEM16(0x88)
	#define OCR1B   _SFR_MEM16(0x8A)
	#define ICR1    _SFR_MEM16(0x86)
	#define TIMSK1  _SFR_MEM8(0x6F)
	#define TIFR1   _SFR_MEM8(0x36)

While you're hopefully somewhat familiar with what a register is, the concept of a register address was foreign to me at first. We see `_SFR_MEM8` and `_SFR_MEM16`, these are macros to help us access the registers. A _SFR, or Special Function Register, is a memory location specific register that controls the operation of the microcontroller. Unlike the more well known registers in LED examples general-purpose registers, SFRs are dedicated to controlling and monitoring various hardware functions of the microcontroller. In this post, we're using them to directly manipulate timer registers, giving us precise control over timing operations.

More generally, [_SFR_MEM8 and _SFR_MEM16 are found in the AVR sfr_defs.h header file](https://onlinedocs.microchip.com/pr/GUID-317042D4-BCCE-4065-BB05-AC4312DBC2C4-en-US-2/index.html?GUID-4E858AD6-A765-4972-84FE-CD55FC481B2F) and are part of the AVR toolchain and provide a way to access these registers in a portable manner.

For those especially curious, we'll take a look at `TCCR1A` in more detail:

	7    6    5    4    3    2    1    0
	+----+----+----+----+----+----+----+----+
	|COM1A1|COM1A0|COM1B1|COM1B0|   -|   -|WGM11|WGM10|
	+----+----+----+----+----+----+----+----+
	
Like a *port* these registers are 8-bits wide. `COM1<b><f>` (**COMPARE CHANNEL <b> <f>**) where <b> is the channel bank ( A | B ) and <x> is the bit field ( 0 | 1 ). We then see `WGM10` and `WGM11`, these are **WAVEFORM GENERATOR MODE** selectors, whose full detail is well out of scope here! 

##Identifying timer registers - quick definitions!

Note: I have included bitfield maps where I could easily draw them up, some of these timers have deep reaches which makes it a bit too cumbersome to spend a lot of time digging for (sorry)...


**TCCR1A** ( and **TCCR1B** )
These control the timer's mode of operations, a quick github search finds this gem:

	TCCR1A = 0b10100010;  // Example: Fast PWM mode, clear OC1A/OC1B on compare match
	TCCR1B = 0b00011010;  // Example: Fast PWM mode, prescaler 8
	
The bitfield map for **TCCR1A** is above in the previous section, B has a different function:

	7      6      5      4      3      2      1      0
	+------+------+------+------+------+------+------+------+
	| ICNC1| ICES1|  -   | WGM13| WGM12| CS12 | CS11 | CS10 |
	+------+------+------+------+------+------+------+------+
	
The definitions are:

	ICNC1       = Input Capture Noise Canceler
	ICES1       = Input Capture Edge Select
	WGM13 && 12 = Waveform Generation Mode (upper 2 bits)
	CS12  && 10 = Clock Select (Prescaler)


**TCNT1**
This 16-bit register holds the actual timer/counter value. It's incremented or decremented based on the timer's configuration.

	TCNT1 = 0;  // Reset the timer count
	
This register has no bit fields.


**OCR1A** and **OCR1B**
These registers are used for comparison with TCNT1. When a match occurs, it can trigger an interrupt or affect output pins.

	OCR1A = 1000;  // Set compare value for channel A
	OCR1B = 500;   // Set compare value for channel B
	

These registers have no bit fields.	

**ICR1**
Input capture mode or as a top value in certain PWM modes.

	ICR1 = 20000;  // Set top value for PWM mode

This register has no bit fields.

**TIMSK1**
This register enables or disables timer-related interrupts.

	TIMSK1 |= (1 << OCIE1A);  // Enable Timer1 Compare Match A interrupt
		
	7      6      5      4      3      2      1      0
	+------+------+------+------+------+------+------+------+
	|  -   |  -   | ICIE1|  -   |  -   |OCIE1B|OCIE1A|TOIE1 |
	+------+------+------+------+------+------+------+------+
	
Bit field definitions:

	ICIE1  = Input Capture Interrupt Enable
	OCIE1B = Output Compare B Match Interrupt Enable
	OCIE1A = Output Compare A Match Interrupt Enable
	TOIE1  = Timer Overflow Interrupt Enable

**TIFR1**
This register holds the interrupt flags. These flags are set when a timer event occurs and are cleared when the interrupt is serviced or manually by software.

	if(TIFR1 & (1 << OCF1A)) {
		// If Timer1 compare match A occurred
		TIFR1 |= (1 << OCF1A);  // ... Then clear the flag
	}

	7      6      5      4      3      2      1      0
	+------+------+------+------+------+------+------+------+
	|  -   |  -   | ICF1 |  -   |  -   | OCF1B| OCF1A| TOV1 |
	+------+------+------+------+------+------+------+------+

Bit field definitions:

	ICF1  = Input Capture Flag
	OCF1B = Output Compare B Match Flag
	OCF1A = Output Compare A Match Flag
	TOV1  = Timer Overflow Flag
	


**Whew! I have included these bit field maps to give you a detailed view of what each bit in these registers controls. When directly manipulating timers, you'll often be setting or clearing specific bits in these registers to achieve the desired timer behavior.**


##Memory-mapped I/O addresses

In this section, I'd like to introduce you to the concept of memory mapped I/O. These are the exact memory locations where our timer registers live. On the ATmega328P (Uno R3, Nano), these registers are mapped to specific addresses in the microcontroller's memory space.

First we have the aforementioned registers:

    | Register 	| Location 	| Decimal| 
    | ---------	| ---------	| -------| 
	| TCCR1A   	|   0x80   	|   128	 | 
	| TCCR1B   	|   0x81 	| 	129	 | 
	| TCCR1C   	| 	0x82 	| 	130	 | 
	| TCNT1  	| 	0x84 	| 	132	 | 
	| ICR1   	| 	0x86 	| 	134	 | 
	| OCR1A  	| 	0x88 	| 	136	 |
	| OCR1B  	|	0x8A 	| 	138	 | 
	| TIMSK1 	| 	0x6F 	| 	111	 | 
	| TIFR1	    |   0x36 	| 	54 	 | 
	
Recall from above the format for accessing these regions:

	/...
	
	#define OCR1A   _SFR_MEM16(0x88)
	#define OCR1B   _SFR_MEM16(0x8A)
	#define TIMSK1  _SFR_MEM8(0x6F)
	
	/...

Now you can see why we use the macros! Couldn't imagine having to write `_SFR_MEM16(0x88)` at every invocation of `OCR1A`.

Though we'll touch on this more in the next section, you can access these memory locations using pointers, which if you recall is simply a variable representation of a memory address. By invoking a memory location in this manner, we can write directly to the timer register:

	// For example, let's set TCCR1A
	*((volatile uint8_t *)0x80) = 0b10100010;  

	// Another example is to create a pointer to read TCNT1
	uint16_t timerValue = *((volatile uint16_t *)0x84);  
	
*Please Note*: When you use this in your own projects, The `volatile` keyword is _crucial_ here as it tells the compiler that the value at this memory location can change unexpectedly. The compiler uses this information to determine how to handle the data here, in our case, the compiler will not attempt to optimize access - reads or writes. All hardware memory addresses are subject to change at any time, despite the best attempts of the programmers due to environmental and even manufacturing defects/hardware interference. 

Understanding these memory-mapped I/O addresses is like having the keys to the kingdom. It allows us to bypass the abstraction layers and communicate directly with the hardware. It's powerful stuff, but remember - with great power comes great responsibility. One wrong move here could throw your entire system into chaos!

#Creating a Timer Definitions Header File

##Purpose of the header file

Now that you've hopefully survived the background information, we can talk about how to use these Timers. First, we're going to create a header file to succinctly hold the Timer registers of interest.

Creating this header file isn't just about keeping things tidy - it's about creating a reusable toolbox for all your future timer manipulation needs. Once you've got this bad boy set up, you'll be slinging timer code like a pro, without having to dig through datasheets every five minutes.

Header files in C++ typically have a .h extension. They're used to:

	Declare functions
	Define constants and macros
	Declare classes and structs
	Share these declarations across multiple source files

Basically - code reuse and simplifying the main program file. 


##Structure and content

The general structure of a header file is:

#ifndef ARDUINO_TIMER_TUTORIAL
#define ARDUINO_TIMER_TUTORIAL

// Your declarations and definitions go here

#endif // ARDUINO_TIMER_TUTORIAL

So we're going to create a header file that references only the registers for the timers we're interested in and then use that to build a simple program at the end of this lesson.

With that in mind, let's start to build out our header file for TIMER1. 

	#ifndef TIMER1_DEFS_H
	#define TIMER1_DEFS_H

	#include <avr/io.h> // Included to avoid violating this post's scope even more
	
	// Timer1 Control Registers
	#define TIMER1_CONTROL_A    TCCR1A
	#define TIMER1_CONTROL_B    TCCR1B

	//
	// ... cut ...
	//
	
	// Bit definitions for TIFR1
	#define ICF1    5
	#define OCF1B   2
	#define OCF1A   1
	#define TOV1    0
	 
	#endif // TIMER1_DEFS_H

To not clutter this post up with the raw header file, I've hosted it on pastebin here: [Timer1.h](https://pastebin.com/VGkhVvh3)

So now we have the start to some code, using this header file you can start to write programs with instructions like:

	#include <Timer1.h>
	
	// ... cut ...
	
	TIMER1_CONTROL_A |= (1 << COM1A1); 

	// .. cut ...
	
Recall that `COM1A1` is a comparison register, specifically, Compare Output Mode for channel A. The above is more readable and much easier to understand than the somewhat more obscure:

	TCCR1A |= (1 << 7); 
	
Which does the same thing.

So while the goal of these threads is to reduce overhead by using hardware level techniques, it's best to not do so at the expense of your sanity. 

I'd also be remiss if I didn't say that we also should use header files for portability reasons, if we went to a Mega2560 MCU, we'd need only change the memory locations in the header and our code would compile just the same as it would on our target R3.


#Timer Configuration from Scratch

Now that we have a pretty good foundation, let's start doing something useful. We'll be referencing the above timer1.h file, so look there if you're curious about what a macro stands for. 

##Understanding timer modes

Recall that Timer1 sets the Waveform Generation Mode (WGM) bits in TCCR1A and TCCR1B, there are 16 total modes of operation for Timer1, they are controlled by the `WGM13:0` bits.

	| Mode | Description                                | WGM13:0 |
	|------|--------------------------------------------|---------|
	| 0    | Normal Mode                                | 0000    |
	| 1    | PWM, Phase Correct, 8-bit                  | 0001    |
	| 2    | PWM, Phase Correct, 9-bit                  | 0010    |
	| 3    | PWM, Phase Correct, 10-bit                 | 0011    |
	| 4    | CTC (Clear Timer on Compare Match) Mode    | 0100    |
	| 5    | Fast PWM, 8-bit                            | 0101    |
	| 6    | Fast PWM, 9-bit                            | 0110    |
	| 7    | Fast PWM, 10-bit                           | 0111    |
	| 8    | PWM, Phase and Frequency Correct (ICR1)    | 1000    |
	| 9    | PWM, Phase and Frequency Correct (OCR1A)   | 1001    |
	| 10   | PWM, Phase Correct (ICR1)                  | 1010    |
	| 11   | PWM, Phase Correct (OCR1A)                 | 1011    |
	| 12   | CTC (ICR1)                                 | 1100    |
	| 13   | Reserved                                   | 1101    |
	| 14   | Fast PWM (ICR1)                            | 1110    |
	| 15   | Fast PWM (OCR1A)                           | 1111    |

**Notes about Timer Modes**

*Normal Mode: Simplest operation, just counts up and overflows.

- CTC Modes - Great for precise timing intervals.

- PWM Modes - Useful for generating PWM signals with various resolutions.

Notes:

Phase Correct provides symmetrical PWM pulses but at half the frequency of Fast PWM.

Using ICR1 as TOP allows OCR1A to be used for PWM output, while using OCR1A as TOP restricts it to defining the counter's upper limit.

To show a quick usage hint, let's set the CTC mode 4:

	TIMER1_CONTROL_A &= ~((1 << WGM11) | (1 << WGM10));
	TIMER1_CONTROL_B &= ~(1 << WGM13);
	TIMER1_CONTROL_B |= (1 << WGM12);
	
If you find the syntax confusing, please review [Bitwise Operations](https://en.wikipedia.org/wiki/Bitwise_operation) [Wikipedia]

The key takeaway here is that understanding these modes is crucial using Timer1's full potential. Each mode has its strengths and is suited for different applications.

##Setting prescalers

First of all, what is a pre-scalar? I analogize them to **gear ratios**, selecting the right prescaler is like choosing the right gear in a car. Too low, and you'll redline your timer before you know it. Too high, and you'll be crawling along, missing all the action. It's all about finding that sweet spot for your specific application.

We employ pre-scalars to extend timer range (big ranges measure more time), save power, and precision.

Timer1 has a few pre-scalar values: 1 (NO pre-scaling), 8, 64, 256, and 1024.

If you refer back to our `TCCR1B` introduction, you will see the `CS12, CS11, and CS10` bits at the end of the register's bit field. We will manipulate those to set a prescalar. Note, the *CS* in *CS*10 stands for Clock Select and we mask those bits to set the tick rate. Please see the reference at the end of this document for all possible ways.

If we were to mask for no pre-scaling (meaning, run at native clock speed):

	TIMER1_CONTROL_B |= (1 << CS10);                  // set CS10 to 1
	TIMER1_CONTROL_B &= ~((1 << CS12) | (1 << CS11)); // clears the CS12 and CS11 bits to 0
	
We clear the CS12 and CS11 bits as a safety step... why? Because in hardware level programming, you make no assumptions.

In a similar way, let's set our prescalar to 1024.

	TIMER1_CONTROL_B |= (1 << CS12) | (1 << CS10); //set CS12 and CS10 to 1.
	TIMER1_CONTROL_B &= ~(1 << CS11);              //clears the CS11 bit to 0
	
##Why do I need to know this?

Let's say we're using a 16 MHz Arduino and we want Timer1 to increment every 1 μs... let's work out our prescalar value and implement it.

	We first write out 16Mhz as 16 000 000 hz
	Then we write out our fraction (1 / 16Mhz) = 6.25e^8, or 62.5 nanoseconds
	Convert nano seconds to microseconds 
	Prescalar = Desired period / system clock period
	Prescalar = 1us / 62.5
	Prescalar = 16
	
Since 16 isn't a valid option (1, 8, 256, or 1024), we simply choose the closest value and run with it. So, 8.

	TIMER1_CONTROL_B |= (1 << CS11);                 
	TIMER1_CONTROL_B &= ~((1 << CS12) | (1 << CS10));
	
If we had a matter where we were using the CTC mode, and desired a 1-second interval, if we attempted to do this with no pre-scalar, we would need to compare values of 16 million (accounting for all cycles per second). We would use the 1024 prescalar here and this gets us down to 15,625 which fits nicely into a 16bit space.


Key take aways:

	Lower prescaler: Higher resolution, shorter maximum time
	Higher prescaler: Lower resolution, longer maximum time
	
It's a balancing act between resolution, maximum time, and the specific timing needs of your application.


	


##Configuring compare match and overflow interrupts

Let's now talk about how we can start triggering the timer. This section will be more code type examples rather than my humdrum explainations. I've prepared code snippets of each of the following generalized topics.

	1. Compare Match A (OCR1A)
	2. Compare Match B (OCR1B)
	3. Input Capture
	4. Overflow

*Configuring Compare Match Interrupt:*

This first example uses a comparison to seek the value of the TIMER1, we will introduce a function in this section call `sei()`, you can [read about sei() here](https://onlinedocs.microchip.com/pr/GUID-317042D4-BCCE-4065-BB05-AC4312DBC2C4-en-US-2/index.html?GUID-4E858AD6-A765-4972-84FE-CD55FC481B2F) but it is a function that "Enables interrupts by setting the global interrupt mask", use this with caution as the source material also warns us that it "implies a memory barrier which can cause additional loss of optimization" [ See footnotes at end of post ].



Tasks:

*Set the **compare to** value in OCR1A register.

*Enable the interrupt in the TIMSK1 register.

*Set up an Interrupt Service Routine (ISR).


    #include <timer1.h>
	
	// ... cut ... 
	
	TIMER1_COMPARE_A = 2000;             // 16MHz/8/2000 = 1kHz = 1ms
	TIMER1_INT_MASK |= (1 << OCIE1A);

	// ... cut ... 
	sei();  // Enable global interrupts

	
	ISR(TIMER1_COMPA_vect) { // sei() gives us access to these vector functions.
	
		// This is analogous to the loop() function we're familiar with, per COMPARE_A, it will cycle every 1ms. 
		// In practical terms, that means an LED would blink every microsecond.
	}


**Configuring and using the Overflow Interrupt**

Tasks:

*Enable the overflow interrupt in the TIMSK1 register.

*Set up an Interrupt Service Routine (ISR).

    #include <timer1.h>
	
	// ... cut ... 
	TIMER1_INT_MASK |= (1 << TOIE1);

	// ... cut ... 
	sei();  

	ISR(TIMER1_OVF_vect) {
		// This code runs when Timer1 overflows
		// With no prescaler on a 16MHz Arduino, this happens about every 4.1ms (65536/16MHz)
	}


**Combining Interrupts**

It is possible to combine both interrupt types in a single program, you just reference them individually [NOTE: Test nesting].

	TIMER1_INT_MASK |= (1 << OCIE1A) | (1 << TOIE1);

	// Compare
	ISR(TIMER1_COMPA_vect) {
		// ...
	}
	
	// Overflow
	ISR(TIMER1_OVF_vect) {
		// ...
	}

Key takeaways:

Key Points:

- When an interrupt occurs, a flag is set in the TIFR1 register. These are automatically cleared when the ISR runs, or you can clear them manually.

- If multiple interrupts occur simultaneously, there's a fixed priority: Input Capture > Compare Match A > Compare Match B > Overflow.

- Keep your ISRs as short and fast as possible. Long-running ISRs can cause timing issues and make your system less responsive.

- If you're sharing variables between ISRs and main code, declare them as `volatile` to ensure proper access.





#Code Snippets and Examples

##Illustrative snippets for each concept

To sort of demonstrate some of these concepts, I will include an "real life" example from my own studies. This sketch simply flashes the onboard LED on an Arduino nano (pin 13). If Wokwi had an oscilloscope, we'd attach it to pin 8 and it would show a square wave.

[You can view the demonstration and code here.](https://wokwi.com/projects/407487610175451137)

##Arduinoesque Timer Implementation

Now, the LED example is fine, but this will give you some insight into the inner workings of our Arduino libraries.

First, recall we set up a timer simply with:

	void timer0_init() {
		TCCR0A = (1 << WGM01) | (1 << WGM00);  // Fast PWM mode
		TCCR0B = (1 << CS02);  				   // Prescaler 256
		TIMSK0 = (1 << TOIE0);  			   // Enable overflow interrupt
		sei();  
	}

Now, having said that, let's dig into `millis()`, the preferred timing function, which tracks milliseconds and is non-blocking, so it keeps things running relatively as it durates..

The ISR for counting milliseconds is very simple:

	volatile unsigned long timer0_millis = 0;
	volatile unsigned char timer0_fract = 0;

	ISR(TIMER0_OVF_vect) {
		unsigned long m = timer0_millis;
		unsigned char f = timer0_fract;

		m += MILLIS_INC;
		f += FRACT_INC;
		if (f >= FRACT_MAX) {
			f -= FRACT_MAX;
			m += 1;
		}

		timer0_fract = f;
		timer0_millis = m;
	}

We can see from the function called `TIMER0_OVF_vect` that we're working with an overflow timer. The current values of `timer0_millis` and `timer0_fract` are copied into local variables `m` and `f` for manipulation. `m` is incremented by MILLIS_INC, which represents the integer millisecond increment each time the timer overflows and `f` is incremented by FRACT_INC, representing the fractional millisecond increment.

The ISR checks if `f` has reached or exceeded FRACT_MAX. If it has, it means the fractional part has accumulated enough to add another millisecond to `m`. `f` is reduced by FRACT_MAX (bringing it back within the correct range), and `m` is incremented by 1 to account for the full millisecond. Finally, the updated values of `m` and `f` are written back to `timer0_millis` and `timer0_fract`. **And that**, in a nut-shell, is the foundation of `millis()`!

Now, we can build on this with the proper function:

	unsigned long millis() {
		unsigned long m;
		uint8_t oldSREG = SREG;
		cli();  // Disable interrupts
		m = timer0_millis;
		SREG = oldSREG;  // Restore interrupt state
		return m;
	}

So first, we store `SREG`, the Status Register, which includes the Global Interrupt Enable flag (I-bit). Saving this state allows the function to restore interrupts to their original state after modifying them.

The second function we pass is cli() which literally translates to "Clear Interrupts" and does as its name implies.

We move on to `m = timer0_millis`, which is updated by the ISR in the above section of this section, this is the number of elapsed milliseconds. 

Once complete, we restore the Status Register and program flow procedes.  If you read this and think "I thought you said it's 'non-blocking', but it stops all of the interrupts..." you'd be right, we consider millis() to be non-blocking because it halts interrupts as it needs to get the value of the overflow counter. With this in mind, it's considered non-blocking because it halts program flow while it copies an external variable into its local execution path. It then restores the interrupts and processing continues. 

We can contrast that with `delay()`:

	void delay(unsigned long ms) {
		uint32_t start = millis();
		while (millis() - start <= ms)
			;  // Wait until the specified time has passed
	}

`delay()` doesn't need a lot of explaination, it simply holds `millis()` and waits for the correct number of microseconds to elapse. `delay()` is considered a blocking function because it does not directly invoke the interrupts and instead waits for a timing function to return its value. 

Hopefully this helps you understand why everyone will recommend `millis()` over `delay()` in your sketches.

##Traffic light w/ Pedestrian button.

I'm sure you've all been at a crosswalk and pressed the button eagerly hoping to get across a busy road faster, ever wonder how those work?

[This is a pretty comprehensive sample of a traffic light system](https://wokwi.com/projects/407493956117131265), I made a bunch of helper functions to clearly illustrate what each step does and populated them with the appropiate register bit shifts.


##Multiple Timers - Dimming LEDs

One of the more interesting aspects of timing is using many together. In this example, Timer1 is set up for 10-bit PWM to control LED brightness, while Timer2 triggers a mode change every second. The idea is to create a 

This is a [dimming LED example](https://wokwi.com/projects/407555272630754305), in it, we are employing Timer1 through the TCCR1* registers to set the PWM. Timer2 is used oscillate the LED though its modes on one-second intervals. We employ an unused pin as a global state tracker. 

##HPET - High Precision Event Timing

I'd be remiss if I didn't include an HPET in any post about timers. An [HPET](https://en.wikipedia.org/wiki/High_Precision_Event_Timer) on PCs is a module in the CPU construction. Sadly, our trust 328p has no hardware HPET, so we must build one when we need it.

To build an HPET, we need to track a hardware event and keep track of its duration. We will build a sketch on Wokwi with a push-button connected to Pin 8 and build an HPET to tell us how long we pressed the button for. In this example, I'll use the deconstructed `millis()` from earlier in this post so you can see it in action.

[You can view the code and play with the example on Wokwki here.](https://wokwi.com/projects/407556590492781569)

##Frequency synthesizer via PWM

I have always been a fan of EDM, so this was a project I tackled early on when learning to program. But this was my first time experimenting with it on an Arduino. So what this project uses is two slide switches to replicate the three position switch I had in my Radioshack kit. The truth table for these is:

*This project is this post's coup d'grace, it shows nearly everything we covered here, in detail, and I've gone to great lengths to make it as accessible as possible by encapsulating as many manipulations in helper functions. My goal in this example was to have you read through the main execution path and then go back and see how it works as you go.*

So what does it do? [First, the project is also hosted on Wokwi here...]()

Now, what does it do?

	1. We use two on/off switches to get four modes of operation 
	 
	 - Morse code (because why not?)
	 
	 - Melody

	 - Siren

	 - Theremin

	2. We illustrate the hardware level buffers for writing to the serial out.

	3. We illustrate the a serial counter to time 1sec serial-out updates. 

	4. We use a slide potentiometer to:

		a. Control the pace of the morse code (5wpm -> 50wpm)
		b. Control the frequency of all other sound modes
		
	5. Illustrates the most heavily used concepts

		a. Overflow for the siren
		b. Compare/match 
		c. Port/register masking and manipulation

	6. Illustrates good "clean coding" practices (this is actually written in accordance with my workplace coding doctrine).


Thank you for reading through this. 


#Appendicies and further charts, reading, etc.

##All Registers discussed in this post:

	+----------+-------------------+--------+------------------------------------------+
	| Register | Name              | Address| Function                                 |
	+----------+-------------------+--------+------------------------------------------+
	| TCCR1A   | Timer/Counter     | 0x80   | Control Register A: Sets compare output  |
	|          | Control Register A|        | mode and waveform generation mode (low)  |
	+----------+-------------------+--------+------------------------------------------+
	| TCCR1B   | Timer/Counter     | 0x81   | Control Register B: Sets waveform        |
	|          | Control Register B|        | generation mode (high) and prescaler     |
	+----------+-------------------+--------+------------------------------------------+
	| TCNT1    | Timer/Counter1    | 0x84   | Contains the current timer/counter value |
	+----------+-------------------+--------+------------------------------------------+
	| OCR1A    | Output Compare    | 0x88   | Contains the compare value for compare   |
	|          | Register A        |        | match on channel A                       |
	+----------+-------------------+--------+------------------------------------------+
	| OCR1B    | Output Compare    | 0x8A   | Contains the compare value for compare   |
	|          | Register B        |        | match on channel B                       |
	+----------+-------------------+--------+------------------------------------------+
	| ICR1     | Input Capture     | 0x86   | Captures timer value when an event       |
	|          | Register          |        | occurs on ICP1 pin                       |
	+----------+-------------------+--------+------------------------------------------+
	| TIMSK1   | Timer/Counter1    | 0x6F   | Timer Interrupt Mask Register: Enables   |
	|          | Interrupt Mask    |        | timer-specific interrupts                |
	+----------+-------------------+--------+------------------------------------------+
	| TIFR1    | Timer/Counter1    | 0x36   | Timer Interrupt Flag Register: Indicates |
	|          | Interrupt Flag    |        | pending timer interrupts                 |
	+----------+-------------------+--------+------------------------------------------+

##Timer1 Interrupt Vector Table

	+-------------------+-------------------+----------------------------------+
	| Interrupt Vector  | Vector Address    | Description                      |
	+-------------------+-------------------+----------------------------------+
	| TIMER1_CAPT_vect  | 0x0010 (0x0020)   | Timer1 Capture Event             |
	| TIMER1_COMPA_vect | 0x0012 (0x0024)   | Timer1 Compare Match A           |
	| TIMER1_COMPB_vect | 0x0014 (0x0026)   | Timer1 Compare Match B           |
	| TIMER1_OVF_vect   | 0x0016 (0x0028)   | Timer1 Overflow                  |
	+-------------------+-------------------+----------------------------------+

##Clock Select Pin Values

	+-------------------+-----------------------------------------------+
	| CS12, CS11, CS10  | Description                                   |
	+-------------------+-----------------------------------------------+
	| 000               | No clock source (Timer/Counter stopped)       |
	| 001               | No prescaling (clock runs at full speed)      |
	| 010               | Clock divided by 8                            |
	| 011               | Clock divided by 64                           |
	| 100               | Clock divided by 256                          |
	| 101               | Clock divided by 1024                         |
	| 110               | External clock source on T1 pin, falling edge |
	| 111               | External clock source on T1 pin, rising edge  |
	+-------------------+-----------------------------------------------+

##Calculating the approximate prescalar:

	+---------------+-----------------------------+-----------------------------+
	| Clock Source  | Timer Frequency             | Overflow Frequency          |
	+---------------+-----------------------------+-----------------------------+
	| 16 MHz        | 16 MHz / Prescaler          | (16 MHz / Prescaler) / 65536|
	| 8 MHz         | 8 MHz / Prescaler           | (8 MHz / Prescaler) / 65536 |
	+---------------+-----------------------------+-----------------------------+

	
##Timer1 Prescalar Selection Table

	+------+------+------+------------------------+----------------------------+
	| CS12 | CS11 | CS10 | Description            | Prescaler Division Factor  |
	+------+------+------+------------------------+----------------------------+
	|  0   |  0   |  0   | No clock source        | Timer/Counter stopped      |
	|  0   |  0   |  1   | No prescaling          | 1                          |
	|  0   |  1   |  0   | Clock / 8              | 8                          |
	|  0   |  1   |  1   | Clock / 64             | 64                         |
	|  1   |  0   |  0   | Clock / 256            | 256                        |
	|  1   |  0   |  1   | Clock / 1024           | 1024                       |
	|  1   |  1   |  0   | External clock (T1)    | Falling edge               |
	|  1   |  1   |  1   | External clock (T1)    | Rising edge                |
	+------+------+------+------------------------+----------------------------+

##Prescalar Effects on Resolution

	+------------+-----------------+---------------------+----------------------+
	| Prescaler  | Timer Frequency | Timer Resolution    | Max Measurable Time  |
	| Value      | (16 MHz clock)  |                     |                      |
	+------------+-----------------+---------------------+----------------------+
	| 1          | 16 MHz          | 0.0625 µs           | 4.096 ms             |
	| 8          | 2 MHz           | 0.5 µs              | 32.768 ms            |
	| 64         | 250 kHz         | 4 µs                | 262.144 ms           |
	| 256        | 62.5 kHz        | 16 µs               | 1.048576 s           |
	| 1024       | 15.625 kHz      | 64 µs               | 4.194304 s           |
	+------------+-----------------+---------------------+----------------------+

## Timer1 WGM Selection Table:

	+------+------+------+------+----------------+----------+------------------+
	| WGM13| WGM12| WGM11| WGM10| Timer/Counter  | TOP      | Update of OCR1x  |
	|      |      |      |      | Mode of        |          | at               |
	|      |      |      |      | Operation      |          |                  |
	+------+------+------+------+----------------+----------+------------------+
	|  0   |  0   |  0   |  0   | Normal         | 0xFFFF   | Immediate        |
	|  0   |  0   |  0   |  1   | PWM, Phase     | 0x00FF   | TOP              |
	|      |      |      |      | Correct, 8-bit |          |                  |
	|  0   |  0   |  1   |  0   | PWM, Phase     | 0x01FF   | TOP              |
	|      |      |      |      | Correct, 9-bit |          |                  |
	|  0   |  0   |  1   |  1   | PWM, Phase     | 0x03FF   | TOP              |
	|      |      |      |      | Correct, 10-bit|          |                  |
	|  0   |  1   |  0   |  0   | CTC            | OCR1A    | Immediate        |
	|  0   |  1   |  0   |  1   | Fast PWM, 8-bit| 0x00FF   | BOTTOM           |
	|  0   |  1   |  1   |  0   | Fast PWM, 9-bit| 0x01FF   | BOTTOM           |
	|  0   |  1   |  1   |  1   | Fast PWM,10-bit| 0x03FF   | BOTTOM           |
	|  1   |  0   |  0   |  0   | PWM, Phase and | ICR1     | BOTTOM           |
	|      |      |      |      | Frequency      |          |                  |
	|      |      |      |      | Correct        |          |                  |
	|  1   |  0   |  0   |  1   | PWM, Phase and | OCR1A    | BOTTOM           |
	|      |      |      |      | Frequency      |          |                  |
	|      |      |      |      | Correct        |          |                  |
	|  1   |  0   |  1   |  0   | PWM, Phase     | ICR1     | TOP              |
	|      |      |      |      | Correct        |          |                  |
	|  1   |  0   |  1   |  1   | PWM, Phase     | OCR1A    | TOP              |
	|      |      |      |      | Correct        |          |                  |
	|  1   |  1   |  0   |  0   | CTC            | ICR1     | Immediate        |
	|  1   |  1   |  0   |  1   | (Reserved)     | -        | -                |
	|  1   |  1   |  1   |  0   | Fast PWM       | ICR1     | BOTTOM           |
	|  1   |  1   |  1   |  1   | Fast PWM       | OCR1A    | BOTTOM           |
	+------+------+------+------+----------------+----------+------------------+

##Timer Application Quick-Reference

	+------------------+-------------------+----------------------------------+
	| Application      | Timer Mode        | Typical Configuration            |
	+------------------+-------------------+----------------------------------+
	| Precise Delays   | CTC               | Use OCR1A for compare value      |
	| PWM Generation   | Fast PWM          | Adjust OCR1A/B for duty cycle    |
	| Frequency Meas.  | Input Capture     | Use rising/falling edge trigger  |
	| Event Counting   | Normal Mode       | Count overflows in ISR           |
	| Software Timers  | CTC or Normal     | Use interrupts to update counters|
	| Motor Control    | Phase Correct PWM | Adjust OCR1A/B for speed control |
	+------------------+-------------------+----------------------------------+

##Common AVR/Arduino Bit Manipulations:

	+-------------------------------+---------------------------------+-------------------------------------------+--------------------------------------------------+
	| Operation                      | C Syntax                          | Example                                   | Description                                      |
	+-------------------------------+----------------------------------+-------------------------------------------+--------------------------------------------------+
	| Set a bit                      | REGISTER |= (1 << BIT)            | TCCR1A |= (1 << COM1A1);                  | Set the bit BIT in REGISTER to 1.            |
	| Clear a bit                    | REGISTER &= ~(1 << BIT)           | TCCR1A &= ~(1 << COM1A0);                 | Clear the bit BIT in REGISTER to 0.          |
	| Toggle a bit                   | REGISTER ^= (1 << BIT)            | PORTB ^= (1 << PB5);                      | Toggle the bit BIT in REGISTER.              |
	| Check if set                  | if (REGISTER & (1 << BIT))        | if (TIFR1 & (1 << TOV1))                  | Check if bit BIT in REGISTER is set (1).     |
	| Check if cleared              | if (!(REGISTER & (1 << BIT)))     | if (!(PIND & (1 << PD2)))                 | Check if bit BIT in REGISTER is cleared (0). |
	| Set multiple bits             | REGISTER |= (BITS_MASK)           | PORTB |= (1 << PB5) | (1 << PB6);          | Set multiple bits specified by BITS_MASK.    |
	| Clear multiple bits           | REGISTER &= ~ (BITS_MASK)         | PORTB &= ~(1 << PB5 | 1 << PB6);         | Clear multiple bits specified by BITS_MASK.  |
	| Toggle multiple bits          | REGISTER ^= (BITS_MASK)           | PORTB ^= (1 << PB5 | 1 << PB6);          | Toggle multiple bits specified by BITS_MASK. |
	| Check if multiple bits are set| if (REGISTER & (BITS_MASK))       | if (TIFR1 & (1 << TOV1 | 1 << OCF1A))    | Check if any of the bits in BITS_MASK are set. |
	| Check if multiple bits are cleared | if (!(REGISTER & (BITS_MASK))) | if (!(PIND & (1 << PD2 | 1 << PD3)))    | Check if all of the bits in BITS_MASK are cleared. |
	| Extract bit value             | bit_value = (REGISTER >> BIT) & 1 | bit_value = (PIND >> PD2) & 1            | Extract the value of a specific bit.             |
	| Set a bit using mask          | REGISTER |= BIT_MASK              | PORTB |= (1 << PB5 | 1 << PB6);          | Set bits specified by BIT_MASK in REGISTER. |
	| Clear a bit using mask        | REGISTER &= ~BIT_MASK             | PORTB &= ~(1 << PB5 | 1 << PB6);         | Clear bits specified by BIT_MASK in REGISTER. |
	| Toggle a bit using mask       | REGISTER ^= BIT_MASK              | PORTB ^= (1 << PB5 | 1 << PB6);          | Toggle bits specified by BIT_MASK in REGISTER. |
	| Check if bits are set using mask | if (REGISTER & BIT_MASK)         | if (TIFR1 & (1 << TOV1 | 1 << OCF1A))    | Check if any bits in BIT_MASK are set.        |
	| Check if bits are cleared using mask | if (!(REGISTER & BIT_MASK))   | if (!(PIND & (1 << PD2 | 1 << PD3)))    | Check if all bits in BIT_MASK are cleared.    |
	+-------------------------------+----------------------------------+-------------------------------------------+--------------------------------------------------+
