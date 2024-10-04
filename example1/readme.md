# Potentiometer example

This example shows how a potentiometer can be used to alter a dataref in the simulator. If you have never used a potentiometer before
then try the Arduino examples under _03.Analog_ in the Arduino IDE. Basically we use the `analogRead()` function to get a value from
the analog input, which will be a number from 0 to 1023, related to the position of the potentiometer. It's up to you to interpret what
that means. For example, it could be a control knob from _min_ to _max_, or it could be a deviation either side of a nominal zero, or centre, point.

Datarefs in X-Plane use floating point values, and a lot of the dataref values range from 0.0 for the minium value to 1.0 for the maximum
value. You could also consider these as percent/100, so 0% is 0.0, 100% is 1.0, and 50% is 0.5. If you are going to use a potentiometer to
control a dataref in X-Plane you should observe the behaviour of the dataref in X-Plane as you move the simulated control on-screen. You should
also confirm that you can write a new value to the dataref and that it will change the control and its effect. Some datarefs are read-only, so it's
not always possible to control the simulated control this way. You will write code to convert the range of values from the `analogRead()`
function to the corresponding value in the simulator when you move the physical control.

This example is written for the Challenger 300 aircraft in X-Plane by DDen. The dataref I am using is `cl300/gshldl_h`, which controls the
brightness of the lighting in Zone 1 in the cockpit. Zone 1 is basically the left side of the cockpit glare shield, and the compass illumination.
The brighness control is a potentiometer on the **COCKPIT LIGHTS** panel, which is near the Pilot's left elbow. If you watch the dataref whilst
turning the simulator control knob you can see the value move from 0.0 at the minimum (anticlockwise) position to 1.0 at the maximum (clockwise)
position. These are also marked 'DIM' and 'BRT' on the panel itself. If you write a new value to this dataref then the knob in the simulator will
move to the new position, and the brightness of the illumination will change.

To write this code we make a copy of the _ardxp_ framework and add code to read a potentiometer and write its value to the simulator. In addition,
code is present so that a new value is only written if it changes. At startup, a change is forced, so that the current value of the potentiometer
is written to the simulator.

Because the value written to the simulator is a float (0.0 to 1.0), and the value returned from the analogue input is an integer (0 to 1023) we use
a simple formula to convert the analogue reading to a float before sending it to the simulator. We just divide by 1023.
