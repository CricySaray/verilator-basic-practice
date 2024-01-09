// Verilator Example
// Norbertas Kremeris 2021
#include <stdlib.h>
#include <iostream>
#include <verilated.h>
#include <verilated_vcd_c.h>
#include "Valu.h"
#include "Valu___024unit.h"

#define MAX_SIM_TIME 20
vluint64_t sim_time = 0;
// variable for counting positive clock edges
vluint64_t posedge_cnt = 0;

void dut_reset (Valu *dut, vluint64_t &sim_time){
	dut->rst = 0;
	if (sim_time >= 3 && sim_time < 6){
		dut->rst = 1;
		dut->a_in = 0;
		dut->b_in = 0;
		dut->op_in = 0;
		dut->in_valid = 0;
	}
}

int main(int argc, char** argv, char** env) {
	// make our testbench initialize signals to random values
	Verilated::commandArgs(argc, argv);

	Valu *dut = new Valu; // instantiates converted ALU module
												// In systemVerilog = `alu dut(.*);`

	// set up the waveform dumping
	Verilated::traceEverOn(true);
	VerilatedVcdC *m_trace = new VerilatedVcdC;
	dut->trace(m_trace, 5); // pass m_trace on to `dut`
	m_trace->open("waveform.vcd");

	// make the simulation happen: (The Body of Simulation)
	while (sim_time < MAX_SIM_TIME) {
		// dut_reset function
		dut_reset(dut, sim_time);

		dut->clk ^= 1; // here is ingenious.
		dut->eval();

		dut->in_valid = 0;
		if (dut->clk == 1){
			posedge_cnt++;
			if (posedge_cnt == 5){
				dut->in_valid = 1;  // assert in_valid on 5th cc
			}
			if (posedge_cnt == 7){
				if (dut->out_valid != 1) // check in_valid on 7th cc
					std::cout << "ERROR!" << std::endl;
			}
		}

		m_trace->dump(sim_time);
		sim_time++;
	}

	m_trace->close();
	delete dut;
	exit(EXIT_SUCCESS);
}
