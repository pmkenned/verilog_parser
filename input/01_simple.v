module Foo(
    input clock,
    input reset,
    input [1:0] x,
    output y,
    output z
);

    assign y = (x[0] | x[1]) & z[2];

    Bar b(
        .clock(clock),
        .reset(reset),
        .a(x[0]),
        .b(x[1])
    );

    reg [7:0] w [15:0];
    always @(posedge clock or posedge reset) begin
        if (reset)
            w <= 'ha0;
        else
            w <= x[0];
    end

endmodule

module Bar(
    input clock,
    input reset,
    input a,
    input b,
    output c
);

    assign c = a | b;

endmodule
