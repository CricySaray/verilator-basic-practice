MODULE=alu

.PHONY: sim
sim: waveform.vcd tree

.PHONY: verilate
verilate: .stamp.verilate

.PHONY: build
build: obj_dir/Valu

.PHONY: waves
waves: waveform.vcd
	@echo
	@echo "### WAVES ###"
	$(GTKWAVE_PATH) waveform.vcd &

waveform.vcd: ./obj_dir/V$(MODULE)
	@echo
	@echo "### SIMULATING ###"
	./obj_dir/V$(MODULE) +verilator+rand+reset+2

./obj_dir/V$(MODULE): .stamp.verilate
	@echo 
	@echo "### BUILDING SIM ###"
	make -C obj_dir -f V$(MODULE).mk V$(MODULE)

.stamp.verilate: $(MODULE).sv tb_$(MODULE).cpp
	@echo
	@echo "### VERILATING ###"
	verilator -Wall --trace --x-assign unique --x-initial unique -cc $(MODULE).sv --exe tb_$(MODULE).cpp
	@touch .stamp.verilate

tree:
	@echo 
	@echo "### GENERATING DEPENDENCY TREE ###"
	makefile2graph | dot -Tsvg -o dependency_tree_$(MODULE).svg
	@echo "Successfully generate the tree of dependency of Makefile."

.PHONY: lint
lint: $(MODULE).sv
	verilator --lint-only $(MODULE).sv

.PHONY: clean
clean:
	rm -rf .stamp.*;
	rm -rf ./obj_dir
	rm -rf waveform.vcd
	rm -rf *.svg

