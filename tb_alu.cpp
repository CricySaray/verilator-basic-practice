// Verilator Example
// Norbertas Kremeris 2021
#include <stdlib.h>
#include <iostream>
#include <verilated.h>
#include <verilated_vcd_c.h>
#include "Valu.h"
#include "Valu___024unit.h"

#define MAX_SIM_TIME 40
vluint64_t sim_time = 0;

int main(int argc, char** argv, char** env) {
	Valu *dut = new Valu; // instantiates converted ALU module
												// In systemVerilog = `alu dut(.*);`

	// set up the waveform dumping
	Verilated::traceEverOn(true);
	VerilatedVcdC *m_trace = new VerilatedVcdC;
	dut->trace(m_trace, 5); // pass m_trace on to `dut`
	m_trace->open("waveform.vcd");

	// make the simulation happen: (The Body of Simulation)
	while (sim_time < MAX_SIM_TIME) {
		dut->clk ^= 1; // here is ingenious.
		dut->eval();
		m_trace->dump(sim_time);
		sim_time++;
	}

	m_trace->close();
	delete dut;
	exit(EXIT_SUCCESS);
}
