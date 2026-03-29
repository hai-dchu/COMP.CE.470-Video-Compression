/*
   Hai Chu 152194657, Chi Mai 151395240

   Estimated hours used per student: 4

  [ ] AI Used in solving the exercise
      - AI was not used in the exercise
*/

#include <iostream>
#include <deque>
#include <vector>
#include <fstream>
#include <cassert>
#include <map>

#include <cstdint>
#include <cstring>

// SKIP THESE AND GO TO THE END FOR MAIN TASK

#define CTX_PROB_BITS 15
#define CTX_PROB_BITS_0 10
#define CTX_PROB_BITS_1 14
#define CTX_MASK_0 (~(~0u << CTX_PROB_BITS_0) << (CTX_PROB_BITS - CTX_PROB_BITS_0))
#define CTX_MASK_1 (~(~0u << CTX_PROB_BITS_1) << (CTX_PROB_BITS - CTX_PROB_BITS_1))
#define CTX_GET_STATE(ctx) ( (ctx)->state[0]+(ctx)->state[1] )
#define CTX_STATE(ctx) ( CTX_GET_STATE(ctx)>>8 )
#define CTX_SET_STATE(ctx, state) {\
  (ctx)->state[0]=(state >> 1) & (int)CTX_MASK_0;\
  (ctx)->state[1]=(state >> 1) & (int)CTX_MASK_1;\
}
#define CTX_MPS(ctx) (CTX_STATE(ctx)>>7)
#define CTX_LPS(ctx,range) ((uint8_t)( ((((CTX_STATE(ctx)&0x80) ? (CTX_STATE(ctx)^0xff) : (CTX_STATE(ctx))) >>2)*(range>>5)>>1)+4  ))
#define CTX_UPDATE(ctx,bin) { \
  int rate0 = (ctx)->rate >> 4;\
  int rate1 = (ctx)->rate & 15;\
\
  (ctx)->state[0] -= ((ctx)->state[0] >> rate0) & (int)CTX_MASK_0;\
  (ctx)->state[1] -= ((ctx)->state[1] >> rate1) & (int)CTX_MASK_1;\
  if (bin) {\
    (ctx)->state[0] += (0x7fffu >> rate0) & (int)CTX_MASK_0;\
    (ctx)->state[1] += (0x7fffu >> rate1) & (int)CTX_MASK_1;\
  }\
}

#define CTX_SET_LOG2_WIN(ctx, size) { \
  int rate0 = 2 + ((size >> 2) & 3); \
  int rate1 = 3 + rate0 + (size & 3);\
 (ctx)->rate = 16 * rate0 + rate1;\
}

const uint8_t tau_g_auc_renorm_table[32] =
{
  6, 5, 4, 4, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};


typedef struct
{
  uint16_t  state[2];
  uint8_t  rate;
} cabac_ctx_t;


class CABAC_Decoder {
  public:

    CABAC_Decoder() {
      m_ctx = nullptr;

    }

    void init(std::deque<uint8_t> *data) {
      m_data = data;
      m_range       = 510;
      m_low       = (readByte() << 8) + readByte();
      m_bitsNeeded  = -8;
    }
    uint8_t readByte() {
      if (m_data->size()) {
        uint8_t byte = m_data->front();
        m_data->pop_front();
        return byte;
      } else {        
        assert(false || "No more data to read");
        return 0;
      }
    }

    unsigned decodeAlignedBinsEP( unsigned numBins )
    {

      unsigned remBins = numBins;
      unsigned bins    = 0;
      while(   remBins > 0 )
      {
        unsigned binsToRead = std::min<unsigned>( remBins, 8 ); //read bytes if able to take advantage of the system's byte-read function
        unsigned binMask    = ( 1 << binsToRead ) - 1;
        unsigned newBins    = (m_low >> (15 - binsToRead)) & binMask;
        bins                = ( bins    << binsToRead) | newBins;
        m_low             = (m_low << binsToRead) & 0x7FFF;
        remBins            -= binsToRead;
        m_bitsNeeded       += binsToRead;
        if( m_bitsNeeded >= 0 )
        {
          m_low |= readByte() << m_bitsNeeded;
          m_bitsNeeded     -= 8;
        }
      }
      return bins;
    }

    unsigned decodeBinsEP( unsigned numBins )
    {
      if (m_range == 256)
      {
        return decodeAlignedBinsEP( numBins );
      }
      unsigned remBins = numBins;
      unsigned bins    = 0;
      while(   remBins > 8 )
      {
        m_low              = (m_low << 8) + (readByte() << (8 + m_bitsNeeded));
        unsigned scaledRange = m_range << 15;
        for( int i = 0; i < 8; i++ )
        {
          bins += bins;
          scaledRange >>= 1;
          if (m_low >= scaledRange)
          {
            bins    ++;
            m_low -= scaledRange;
          }
        }
        remBins -= 8;
      }
      m_bitsNeeded   += remBins;
      m_low <<= remBins;
      if( m_bitsNeeded >= 0 )
      {
        m_low += readByte() << m_bitsNeeded;
        m_bitsNeeded -= 8;
      }
      unsigned scaledRange = m_range << (remBins + 7);
      for ( int i = 0; i < remBins; i++ )
      {
        bins += bins;
        scaledRange >>= 1;
        if (m_low >= scaledRange)
        {
          bins    ++;
          m_low -= scaledRange;
        }
      }
      return bins;
    }

    uint8_t decodeBit() 
    {
      unsigned      bin  = CTX_MPS(m_ctx);
      uint32_t      lps  = CTX_LPS(m_ctx, m_range);
    
      m_range -= lps;
      uint32_t scaledRange = m_range << 7;
      if (m_low < scaledRange)
      {
        // MPS path
        if (m_range < 256)
        {
          m_range <<= 1;
          m_low <<= 1;
          m_bitsNeeded++;
          if( m_bitsNeeded >= 0 )
          {
            m_low += readByte() << m_bitsNeeded;
            m_bitsNeeded -= 8;
          }
        }
      }
      else
      {
        bin = 1 - bin;
        // LPS path
        int num_bits = tau_g_auc_renorm_table[lps >> 3];
        m_low -= scaledRange;
        m_low = m_low << num_bits;
        m_range = lps << num_bits;
        m_bitsNeeded += num_bits;
        if( m_bitsNeeded >= 0 )
        {
          m_low += readByte() << m_bitsNeeded;
          m_bitsNeeded -= 8;
        }
      }
      CTX_UPDATE(m_ctx, bin );
      return  bin;
    }

    cabac_ctx_t* m_ctx;

  private:
    std::deque<uint8_t> *m_data;
    uint32_t m_range;
    uint32_t m_low;
    int m_bitsNeeded;
    
};


class CABAC_Encoder {
  public:

    CABAC_Encoder() {
      m_ctx = nullptr;      
    }

    void init(std::deque<uint8_t> *data) {
      m_data        = data;
      m_range       = 510;
      m_low         = 0;
      m_bits_left   = 23;
      m_num_buffered_bytes = 0;
      m_buffered_byte = 0xff;
    }

    void cabac_write()
    {
      uint32_t lead_byte = m_low >> (24 - m_bits_left);
      m_bits_left += 8;
      m_low &= 0xffffffffu >> m_bits_left;

      if (lead_byte == 0xff) {
        m_num_buffered_bytes++;
      } else {
        if (m_num_buffered_bytes > 0) {
          uint32_t carry = lead_byte >> 8;
          uint32_t byte = m_buffered_byte + carry;
          m_buffered_byte = lead_byte & 0xff;
          writeByte(byte);

          byte = (0xff + carry) & 0xff;
          while (m_num_buffered_bytes > 1) {
            writeByte(byte);
            m_num_buffered_bytes--;
          }
        } else {
          m_num_buffered_bytes = 1;
          m_buffered_byte = lead_byte;
        }
      }
    }

    void encodeAlignedBinsEP(const uint32_t bin_values, int num_bins) 
    {
      uint32_t rem_bins = num_bins;
      while (rem_bins > 0) {
        unsigned bins_to_code = rem_bins<8?rem_bins:8;
        unsigned bin_mask = (1 << bins_to_code) - 1;
        unsigned new_bins = (bin_values >> (rem_bins - bins_to_code)) & bin_mask;
        m_low = (m_low << bins_to_code) + (new_bins << 8); //range is known to be 256
        rem_bins -= bins_to_code;
        m_bits_left -= bins_to_code;
        if (m_bits_left < 12) {
          cabac_write();
        }
      }
    }

    void encodeBinsEP(uint32_t bin_values, int num_bins)
    {
      uint32_t pattern;
      
      if (m_range == 256) {
        encodeAlignedBinsEP(bin_values, num_bins);
        return;
      }
      while (num_bins > 8) {
        num_bins -= 8;
        pattern = bin_values >> num_bins;
        m_low <<= 8;
        m_low += m_range * pattern;
        bin_values -= pattern << num_bins;
        m_bits_left -= 8;

        if(m_bits_left < 12) {
          cabac_write();
        }
      }
      m_low <<= num_bins;
      m_low += m_range * bin_values;
      m_bits_left -= num_bins;
      if (m_bits_left < 12) {
        cabac_write();
      }
    }

    void encodeBit(uint8_t bin_value) 
    {
      uint32_t lps = CTX_LPS(m_ctx, m_range);

      m_range -= lps;
    
      // Not the Most Probable Symbol?
      if ((bin_value ? 1 : 0) != CTX_MPS(m_ctx)) {
        int num_bits = tau_g_auc_renorm_table[lps >> 3];
        m_low = (m_low + m_range) << num_bits;
        m_range = lps << num_bits;
        
        m_bits_left -= num_bits;
        if (m_bits_left < 12) {
          cabac_write();
        }
      } else {    
        if (m_range < 256) {
          m_low <<= 1;
          m_range <<= 1;
          m_bits_left--;
    
          if (m_bits_left < 12) {
            cabac_write();
          }
        }
      }
      CTX_UPDATE(m_ctx, bin_value);
    }

    void finish() 
    {          
      if (m_low >> (32 - m_bits_left)) {
        writeByte(m_buffered_byte + 1);
        while (m_num_buffered_bytes > 1) {
          writeByte(0);
          m_num_buffered_bytes--;
        }
        m_low -= 1 << (32 - m_bits_left);
      } else {
        if (m_num_buffered_bytes > 0) {
          writeByte(m_buffered_byte);
        }
        while (m_num_buffered_bytes > 1) {
          writeByte(0xff);
          m_num_buffered_bytes--;
        }
      }
      uint32_t val = m_low >> 8;
      for(int bits = 24 - m_bits_left; bits > 0; bits -= 8) {
        writeByte(val >> bits);
      }      
    }

    void writeByte(uint8_t byte) {
      m_data->push_back(byte);
    }

    void encode_bin_trm(const uint8_t bin_value)
    {
      encodeBit(1);
      encodeBit(1);
      encodeBit(0);
      encodeBit(0);
      encodeBit(0);
      m_range -= 2;
      if(bin_value) {
        m_low += m_range;
        m_low <<= 7;
        m_range = 2 << 7;
        m_bits_left -= 7;
      } else if (m_range >= 256) {
        return;
      } else {
        m_low <<= 1;
        m_range <<= 1;
        m_bits_left--;
      }

      if (m_bits_left < 12) {
        cabac_write();
      }
    }

    cabac_ctx_t* m_ctx;

  private:
    std::deque<uint8_t> *m_data;
    int m_range;
    int m_low;
    int m_bits_left;
    int m_num_buffered_bytes;
    uint8_t m_buffered_byte;
};


static void ctx_init(cabac_ctx_t *ctx, int32_t init_value, uint8_t rate)
{

  int slope = (init_value >> 3) - 4;
  int offset = ((init_value & 7) * 18) + 1;
  int inistate = ((slope) >> 1) + offset;
  int state_clip = inistate < 1 ? 1 : inistate > 127 ? 127 : inistate;
  const int p1 = (state_clip << 8);
  ctx->state[0] = p1 & CTX_MASK_0;
  ctx->state[1] = p1 & CTX_MASK_1;
  CTX_SET_LOG2_WIN(ctx, rate);
}


/*
 * MAIN TASK STARTS HERE, YOU CAN MODIFY THE CODE BELOW AS NEEDED
 */

const char* test_ipsum_lorem = "Lorem ipsum dolor sit amet, consectetur adipiscing elit.\n"
"Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\n"
"Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.\n"
"Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur.\n"
"Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.\n"
"Lorem ipsum dolor sit amet, consectetur adipiscing elit.\n"
"Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\n"
"Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.\n"
"Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur.\n"
"Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";



static const uint8_t SAMPLE_FLAG_INIT[2][9] = {
   // Initialization values for the probability of the value being 1, range is 0 to 70 with 35 being the neutral point (50% probability)
  {  70,  70,  70,  50,  35,  20,  50,  50, 35 },
   // Rate of adaptation (how quickly the probability changes based on observed bits), valid range is 0 to 13 with higher values meaning faster adaptation
  {  0,  10,  0,  13,  10,  10,  10,  13, 10 },
};

bool is_special(char c) {
  return c == ' ' || c == 'e' 
      || c == 't' || c == 'o'
      || c == 'a' || c == 'h'
      || c == 'i' || c == 'n'
      || c == 's' || c == 'd'
      || c == 'r' || c == 'l'
      || c == '\r' || c == '\n'
      || c == 'u' || c == '"';
}

int main() {
  // EXAMPLE CONTEXT, CREATE MORE CONTEXTS AS NEEDED FOR BONUS
  cabac_ctx_t ctx[9];
  for(int i = 0; i < 9; i++) {
    ctx_init(&ctx[i], SAMPLE_FLAG_INIT[0][i] /*probability*/, SAMPLE_FLAG_INIT[1][i]/*change rate*/);
  }

  // std::vector<uint8_t> input_data(test_ipsum_lorem, test_ipsum_lorem + strlen(test_ipsum_lorem) - 1);

  // FOR BONUS: read winnie_the_pooh.txt as input data
  std::vector<uint8_t> input_data;
  {
    std::ifstream file("winnie_the_pooh.txt", std::ios::binary);
    if (!file.is_open()) {
      std::cerr << "Error: Could not open file 'winnie_the_pooh.txt'\n";
      return 1;
    }
    input_data = std::vector<uint8_t>(std::istreambuf_iterator<char>(file), {});
  }

  // Context for special character
  // Top 16 most common characters in winnie_the_pooh.txt
  std::map<char, int> special_binarization = {
  {' ', 0b0000}, {'e', 0b0001}, {'t', 0b0010}, {'o', 0b0011},
  {'a', 0b0100}, {'h', 0b0101}, {'i', 0b0110}, {'n', 0b0111},
  {'s', 0b1000}, {'d', 0b1001}, {'r', 0b1010}, {'l', 0b1011},
  {'\r', 0b1100}, {'\n', 0b1101}, {'u', 0b1110}, {'"', 0b1111} 
  };
  

  std::deque<uint8_t> data; // This will hold the encoded data

  size_t input_data_size = input_data.size(); // Number of bytes to encode

  CABAC_Encoder encoder;
  encoder.init(&data);
  

  for(int i = 0; i < input_data_size; i++) {
    bool is_vowel_char = is_special(input_data[i]);
    encoder.m_ctx = &ctx[8];
    encoder.encodeBit(is_vowel_char ? 1 : 0);

    if (is_vowel_char) {
      encoder.encodeBinsEP(special_binarization[input_data[i]], 4);
      continue;
    }
    
    // 8 bits for each character, we can use a different context for each bit position or reuse contexts as needed
    for(int j = 0; j < 8; j++) {
      // We select a different context for each bit position
      encoder.m_ctx = &ctx[j]; // Select context to use !! MUST BE THE SAME IN DECODER IN SAME ORDER !!

      // Encode this bit of the input byte
      encoder.encodeBit(input_data[i] & (1 << (7-j)) ? 1 : 0);
    }
  }

    // IF SOME PROBLEMS WITH ENDING THE STREAM / DECODING FAIL AT THE END, PUSH COUPLE MORE BITS
  // encoder.encodeBit(0);
  // encoder.encodeBit(0);

  encoder.encode_bin_trm(1);
  encoder.finish();

  std::cout << "Original size: " << input_data_size << " bytes" << std::endl;
  std::cout << "Output data size: " << data.size() << " bytes" << std::endl;
  std::cout << "Compression ratio: " << data.size() / static_cast<double>(input_data_size)  << std::endl;


  /*  
   *  Now decode the data and verify that we get the same bits back  
   */
  CABAC_Decoder decoder;

  std::vector<uint8_t> decoded_data;

  // This sections should use ONLY the encoded data and initialization flags as input
  // We can also use input_data_size to know how many bytes we need to decode
  decoder.init(&data);

  cabac_ctx_t dec_ctx[9];
  for(int i = 0; i < 9; i++) {
    ctx_init(&dec_ctx[i], SAMPLE_FLAG_INIT[0][i] /*probability*/, SAMPLE_FLAG_INIT[1][i]/*change rate*/);
  } 

  // JUST AN EXAMPLE, REPLACE WITH OWN DECODING LOGIC FOR BONUS
  for(int i = 0; i < input_data_size; i++) {
    decoder.m_ctx = &dec_ctx[8];
    if (decoder.decodeBit()) {
      int identifier = decoder.decodeBinsEP(4);
        for(auto& pair : special_binarization) {
          if(pair.second == identifier) {
            decoded_data.push_back(pair.first);
            break;
          }
      }
      continue;
    }

    uint8_t decoded_byte = 0;

    for(int j = 0; j < 8; j++) {
      decoder.m_ctx = &dec_ctx[j]; // Select context to use !! MUST BE THE SAME IN ENCODER IN SAME ORDER !!
      int bit = decoder.decodeBit();
      decoded_byte |= (bit << (7-j));
    }

    decoded_data.push_back(decoded_byte);
  }

  // Compare decoded data with original input data and print results
  bool success = (decoded_data == input_data);
  if (success) {
    std::cout << "Decoding successful, data matches original input!" << std::endl;
  } else {
    std::cout << "Decoding failed, data does not match original input." << std::endl;
    for(int i = 0; i < input_data_size; i++) {
      if (decoded_data[i] != input_data[i]) {
        std::cout << "Mismatch at byte " << i << ": expected " << static_cast<int>(input_data[i]) 
                  << ", got " << static_cast<int>(decoded_data[i]) << std::endl;
      }
    }
    // Write the output to a file for further analysis
    std::ofstream out_file("decoded_output.bin", std::ios::binary);
    if (out_file.is_open()) {
      out_file.write(reinterpret_cast<const char*>(decoded_data.data()), decoded_data.size());
      out_file.close();
    } else {
      std::cerr << "Error: Could not open file 'decoded_output.bin' for writing\n";
    }
  }
  

  return 0;
}