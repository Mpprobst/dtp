struct data_pkt_t {
  int type;         // 4B
  int seq_no;       // 4B
  int length;       // 4B
  char data[1024];  // fixed at 10B
};
