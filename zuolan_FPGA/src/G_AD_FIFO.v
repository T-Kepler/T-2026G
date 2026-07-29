module G_AD_FIFO (
    input  [11:0] data,
    input         aclr,
    input         rdclk,
    input         rdreq,
    input         wrclk,
    input         wrreq,
    output [11:0] q,
    output        wrfull
);

  dcfifo dcfifo_component (
      .data      (data),
      .rdclk     (rdclk),
      .rdreq     (rdreq),
      .wrclk     (wrclk),
      .wrreq     (wrreq),
      .q         (q),
      .wrfull    (wrfull),
      .aclr      (aclr),
      .eccstatus (),
      .rdempty   (),
      .rdfull    (),
      .rdusedw   (),
      .wrempty   (),
      .wrusedw   ()
  );

  defparam
      dcfifo_component.intended_device_family = "Cyclone IV E",
      dcfifo_component.lpm_numwords = 16384,
      dcfifo_component.lpm_showahead = "ON",
      dcfifo_component.lpm_type = "dcfifo",
      dcfifo_component.lpm_width = 12,
      dcfifo_component.lpm_widthu = 14,
      dcfifo_component.overflow_checking = "OFF",
      dcfifo_component.rdsync_delaypipe = 4,
      dcfifo_component.underflow_checking = "ON",
      dcfifo_component.use_eab = "ON",
      dcfifo_component.wrsync_delaypipe = 4;

endmodule
