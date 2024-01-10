// Verilator Example
// Norbertas Kremeris 2021
#include <ostream>
#include <stdlib.h>
#include <cstdlib>
#include <iostream>
#include <verilated.h>
#include <verilated_vcd_c.h>
#include "obj_dir/Valu.h"
#include "obj_dir/Valu___024unit.h"

#define MAX_SIM_TIME 300
vluint64_t sim_time = 0;
// variable for counting positive clock edges
vluint64_t posedge_cnt = 0;

void dut_reset (Valu *dut, vluint64_t &sim_time){
	dut->rst = 0;
	if (sim_time >= 3 && sim_time < 6){
		dut->rst      = 1;
		dut->a_in     = 0;
		dut->b_in     = 0;
		dut->op_in    = 0;
		dut->in_valid = 0;
	}
}

// function to check out_valid
#define VERIF_START_TIME 7
void check_out_valid(Valu *dut, vluint64_t &sim_time){
	static unsigned char in_valid      = 0;
	static unsigned char in_valid_d    = 0;
	static unsigned char out_valid_exp = 0;

	if (sim_time >= VERIF_START_TIME) {
		// note the order!
		out_valid_exp = in_valid_d;
		in_valid_d    = in_valid;
		in_valid      = dut->in_valid;
		if (out_valid_exp != dut->out_valid) {
			std::cout << "ERROR: out_valid mismatch, "
				<< "exp: " << (int)(out_valid_exp)
				<< " recv: " << (int)(dut->out_valid)
				<< " simtime: " << sim_time << std::endl;
		}
	}
}

void set_rnd_out_valid(Valu *dut, vluint64_t &sim_time){
	if (sim_time >= VERIF_START_TIME) {
		dut->in_valid = rand() % 2;
	}
}

int main(int argc, char** argv, char** env) {
	// take a seed for random function with time changing
	srand(time(NULL));
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

		if (dut->clk == 1){
			posedge_cnt++;
			dut->in_valid = 0;
			switch(posedge_cnt){
				case 10:
					dut->in_valid = 1;
					dut->a_in = 5;
					dut->b_in = 3;
					dut->op_in = Valu___024unit::operation_t::add;
					break;

				case 12:
					if (dut->out != 8)
						std::cout << "Addition failed @ " << sim_time << std::endl;
					break;

				case 20:
					dut->in_valid = 1;
					dut->a_in = 5;
					dut->b_in = 3;
					dut->op_in = Valu___024unit::operation_t::sub;
					break;

				case 22:
					if (dut->out != 2)
						std::cout << "Subtraction failed @ " << sim_time << std::endl;
					break;

			}
			check_out_valid(dut, sim_time);
		}

		m_trace->dump(sim_time);
		sim_time++;
	}

	m_trace->close();
	delete dut;
	exit(EXIT_SUCCESS);
}
