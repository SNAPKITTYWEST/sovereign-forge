// ============================================================================
// VERILOG SMT REFINEMENT CHECKER & EVALUATOR ACCELERATOR
// Expression: 10 + (?x * 5) == Target Invariant (20)
// Solution:   ?x = 2  (hardware verified)
//
// State machine: IDLE -> EVAL -> CHECK
//   EVAL:  compute 10 + (candidate_x * 5)
//   CHECK: compare result to target_invariant, assert sat_valid
//
// Connects to:
//   synthesis.dsl          (DSSSL software synthesis stage)
//   grove.sgml             (SGML input grove)
//   proofs/lean4/          (sovereign-forge Lean verification)
//   netlister/sov_kernel.c (C kernel linked from Lean proofs)
// ============================================================================

module refinement_eval (
    input  wire        clk,
    input  wire        rst_n,
    input  wire        start,
    input  wire [31:0] candidate_x,
    input  wire [31:0] target_invariant,
    output reg  [31:0] computed_result,
    output reg         sat_valid,
    output reg         busy
);

    localparam IDLE  = 2'b00;
    localparam EVAL  = 2'b01;
    localparam CHECK = 2'b10;

    reg [1:0] state;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state           <= IDLE;
            computed_result <= 32'd0;
            sat_valid       <= 1'b0;
            busy            <= 1'b0;
        end else begin
            case (state)
                IDLE: begin
                    sat_valid <= 1'b0;
                    if (start) begin
                        busy  <= 1'b1;
                        state <= EVAL;
                    end
                end

                EVAL: begin
                    // Hardware execution of SGML tree math: 10 + (candidate_x * 5)
                    computed_result <= 32'd10 + (candidate_x * 32'd5);
                    state           <= CHECK;
                end

                CHECK: begin
                    busy      <= 1'b0;
                    sat_valid <= (computed_result == target_invariant);
                    state     <= IDLE;
                end

                default: state <= IDLE;
            endcase
        end
    end

endmodule

// ============================================================================
// TESTBENCH — iterate candidates 1..5, print SAT/UNSAT per candidate
// Run: iverilog -o sim refinement_eval.v && vvp sim
// Expected output:
//   Candidate ?x = 1 | Computed: 15 | UNSAT
//   Candidate ?x = 2 | Computed: 20 | SAT   <-- solution
//   Candidate ?x = 3 | Computed: 25 | UNSAT
//   Candidate ?x = 4 | Computed: 30 | UNSAT
//   Candidate ?x = 5 | Computed: 35 | UNSAT
// ============================================================================

module tb_refinement_eval;
    reg        clk;
    reg        rst_n;
    reg        start;
    reg  [31:0] candidate_x;
    reg  [31:0] target_invariant;
    wire [31:0] computed_result;
    wire        sat_valid;
    wire        busy;

    refinement_eval uut (
        .clk(clk),
        .rst_n(rst_n),
        .start(start),
        .candidate_x(candidate_x),
        .target_invariant(target_invariant),
        .computed_result(computed_result),
        .sat_valid(sat_valid),
        .busy(busy)
    );

    always #5 clk = ~clk;

    initial begin
        clk = 0; rst_n = 0; start = 0;
        candidate_x = 0; target_invariant = 32'd20;
        #10 rst_n = 1;

        $display("[VERILOG HARDWARE ACCELERATOR SIMULATION]");
        $display("Expression: 10 + (?x * 5) == %0d", target_invariant);
        $display("--------------------------------------------");

        for (candidate_x = 1; candidate_x <= 5; candidate_x = candidate_x + 1) begin
            #10 start = 1;
            #10 start = 0;
            #20;
            $display("  -> Candidate ?x = %0d | Computed: %0d | SMT: %s",
                     candidate_x, computed_result,
                     sat_valid ? "SAT" : "UNSAT");
            if (sat_valid) begin
                $display("[+] HARDWARE PROOF: ?x = %0d satisfies invariant (%0d)",
                         candidate_x, target_invariant);
            end
        end
        $display("--------------------------------------------");
        $finish;
    end
endmodule
