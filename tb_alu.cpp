// Verilator Example
// Norbertas Kremeris 2021
#include <cstddef>
#include <cstdint>
#include <deque>
#include <ostream>
#include <stdlib.h>
#include <cstdlib>
#include <iostream>
#include <verilated.h>
#include <verilated_vcd_c.h>
#include <verilatedos.h>
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

// AluInTx Transaction item
// NOTE: define the test-need data type and content.
class AluInTx {
	public:
		uint32_t a;
		uint32_t b;
		enum Operation {
			add = Valu___024unit::operation_t::add,
			sub = Valu___024unit::operation_t::sub,
			nop = Valu___024unit::operation_t::nop
		} op; // define the op element for interface
};

// for test to define external data
uint32_t count = 0;
// NOTE: generate random input data
AluInTx* rndAluInTx(){
	// 20% chance of generation a transaction
//	if (rand()%5 == 0){
		AluInTx *tx = new AluInTx();
		//our ENUM only has entries with values 0, 1, 2
		tx->op = AluInTx::Operation(/*rand()%3*/count++%3/*this only for test on extreme condition*/); 
		tx->a  = rand() % 11 + 10;
		tx->b  = rand() % 9;
		return tx;
//	} else {
//		return NULL;
//	}
}

class AluInDrv {
	private:
		Valu *dut;
	public:
		AluInDrv(Valu *dut){ // set a Valu pointer to initiate the DUT
			// NOTE: as same, pass data to inside of drive
			this->dut = dut;
		}
		// NOTE: driver need dut and tx(test data) cuz driver is the connector between DUT and input
		void drive(AluInTx *tx){
			// we always start with in_valid set to 0, and set it to 
			// 1 later only if necessary
			dut->in_valid = 0;
			// Don't drive anything if a transaction item doesn't exist
			// NOTE: important! that is, a and b will be pass into dut when op_in is 2'b01 or 2'b10
			if (tx != NULL){
				if (tx->op != AluInTx::nop) {
					// if the operation is not NOP, we drive it onto the 
					// input interface pins.
					// NOTE: in_valid is asserted when op_in is not nop status
					dut->in_valid = 1;
					dut->op_in    = tx->op;
					dut->a_in     = tx->a;
					dut->b_in     = tx->b;
				}
				// Release the memory by deleting the tx item
				// after it has been consumed
				// NOTE: the inside data tx is temporary data for driving DUT
				delete tx;
			}
		}
};

// NOTE: define the type and content of output convenient to scoreboard
class AluOutTx {
	public:
		uint32_t out;
};

// ALU scoreboard
class AluScb { // wait to complete
	private:
		// NOTE: queue structure FIFO that can be regard as data buffer
		std::deque<AluInTx*> in_q; // create a queue

	public:
		// Input interface moniter port
		// NOTE: push the valid input data to queue
		void writeIn(AluInTx *tx){
			// Push the received transaction item into a queue for later
			in_q.push_back(tx);
		}

		// Output interface monitor port
		void writeOut(AluOutTx* tx){
			// We should never get any data from the output interface
			// before an input gets driven to the input interface
			// NOTE: in_q must have data cuz the writeIn is run first.
			if (in_q.empty()){
				std::cout << "Fatal Error in AluScb: empty AluInTx queue" << std::endl;
				exit(1);
			}

			// Grab the transaction item from the front of the input item queue
			AluInTx* in;
			in = in_q.front(); // return the first element of queue
			in_q.pop_front(); // remove the first element of queue

			switch(in->op){
				// A valid signal should not be created at the output when there is no operation,
				// so we should never get a transaction item where the operation is NOP
				case AluInTx::nop :
					std::cout << "Fatal error in AluScb, recevied NOP on input" << std::endl;
					break;

				// Recevied transaction is add
				case AluInTx::add :
					if (in->a + in->b != tx->out) {
						std::cout << std::endl;
						std::cout << "AluScb: add mismatch" << std::endl;
						std::cout << " Expected: " << in->a + in->b
											<< " Actual: " << tx->out << std::endl;
						std::cout << " Simtime: " << sim_time << std::endl;
					}
					break;

				// Recevied transaction is add
				case AluInTx::sub :
					if (in->a - in->b != tx->out) {
						std::cout << std::endl;
						std::cout << "AluScb: sub mismatch" << std::endl;
						std::cout << " Expected: " << in->a - in->b
											<< " Actual: " << tx->out << std::endl;
						std::cout << " Simtime: " << sim_time << std::endl;
					}
					break;
			}
			// As the transaction items were allocated on the heap, it's important
			// to free the memory after they have been used
			delete in;
			delete tx;
		}
};

class AluInMon {
	private:
		Valu *dut;
		AluScb *scb;
	public:
		AluInMon(Valu *dut, AluScb *scb){
			this->dut = dut;
			this->scb = scb;
		}
		// NOTE: the input data is added to new tx of monitor only when in_valid is asserted.
		void monitor(){  
			if (dut->in_valid == 1){
				// If there is valid data at the input interface,
				// create a new AluInTx transaction item and populate
				// it with data observed at the interface pins
				// NOTE: create a new tx different from former, this is data under valid input
				AluInTx *tx = new AluInTx(); 
				tx->op = AluInTx::Operation(dut->op_in);
				tx->a = dut->a_in;
				tx->b = dut->b_in;

				// then pass the transaction item to the scoreboard
				scb->writeIn(tx);
			}
		}
};

class AluOutMon {
	private:
		Valu *dut;
		AluScb *scb;
	public:
		AluOutMon(Valu *dut, AluScb *scb){
			// NOTE: pass the dut from main to inside of Monitor to pick up data!!!
			this->dut = dut;
			this->scb = scb;
		}
		// NOTE: same reason, there is a new tx to accumulate the out_valid out data. 
		void moniter(){
			if (dut->out_valid == 1) {
				// If there is valid data at the output interface,
				// create a new AluOutTx transaction item and populate
				// it with result observed at the interface pins
				AluOutTx *tx = new AluOutTx();
				tx->out = dut->out;

				// then pass the transaction item to the scoreboard
				// NOTE: the valid data from output be passed to scoreboard.
				scb->writeOut(tx);
			}
		}
};


int main(int argc, char** argv, char** env) {
	// take a seed for random function with time changing
	srand(time(NULL));
	// make our testbench initialize signals to random values
	Verilated::commandArgs(argc, argv);

	Valu *dut = new Valu; // instantiates converted ALU module
												// In systemVerilog = `alu dut(.*);`

	// 1. set up the waveform dumping
	Verilated::traceEverOn(true);
	VerilatedVcdC *m_trace = new VerilatedVcdC;
	// NOTE: NOTE: the input test input data is pass into inside of dut only when in_valid is asserted
	// so status of a_in and b_in will keep same with before unless in_valid is asserted again.
	dut->trace(m_trace, 5); // pass m_trace on to `dut`
	m_trace->open("waveform.vcd");

	AluInTx *tx;

	// Here we create the driver, scoreboard, input and output monitor blocks
	// NOTE: four statement below is to pass the main dut into inside of every block connected to DUT
	AluInDrv *drv = new AluInDrv(dut);
	AluScb *scb = new AluScb();
	AluInMon *inMon = new AluInMon(dut, scb);
	AluOutMon *outMon = new AluOutMon(dut, scb);

	while (sim_time < MAX_SIM_TIME) {
		dut_reset(dut, sim_time);
		dut->clk ^= 1;
		dut->eval();

		// Do all the driving/monitoring on a positive edge
		if (dut->clk == 1){
			if (sim_time >= VERIF_START_TIME){
				//Generate a randomized transaction item of type AluInTx
				// NOTE: receive the random test input by return value
				tx = rndAluInTx();

				// Pass the transaction item to the ALU input interface driver,
				// which drives the input interface based on the info in the 
				// transaction item
				drv->drive(tx);

				// Monitor the input interface
				// NOTE: the writeIn function is inside
				inMon->monitor();

				// Mointor the output interface
				// NOTE: the writeOut function is inside
				outMon->moniter();
			}
		}
		// end of positive edge processing

		// 2. dump by sim time
		m_trace->dump(sim_time);
		sim_time++;
	}
	
	delete dut;
	delete outMon;
	delete inMon;
	delete scb;
	delete drv;
	// 3. close trace function
	m_trace->close(); // this line can't throw out, or it will can't open waveform file
	exit(EXIT_SUCCESS);
	

/*
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
*/
}
