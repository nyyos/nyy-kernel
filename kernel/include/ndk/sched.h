#pragma once

enum {
	// ready to be run
	kThreadStateReady = 0,
	// currently running
	kThreadStateRunning,
	// exiting / getting killed
	kThreadStateTerminating,
	// finished terminating
	kThreadStateDead,
};
