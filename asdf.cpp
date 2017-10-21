/* How to compile and run this file:
   COMPILATION: g++ -o bot.exe bot.cpp
   RUNNING: while true; do ./bot.exe; sleep 1; done
*/

/* C includes for networking things */
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

/* C++ includes */
#include <string>
#include <unordered_map>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <sstream>

#include <unistd.h>
#include <sstream>
#include <iterator>
#include <stdlib.h>     /* srand, rand */

using namespace std;


class PortfolioItem {


  public:
  int symbol;
  bool direction;
  int id;
  int price;
  int size;
  bool convert = false;
    PortfolioItem (int s, bool d, int i, int p, int a):symbol (s), direction(d), id (i), price (p),
    size(a)
   {}

  void decrease_size (int decrease)
  {size-= decrease;}
  void increase_size(int increase) {size += increase;}
};


std::vector < std::string > split (std::string const &input)
{
  std::istringstream buffer (input);
  std::vector < std::string >
  ret ((std::istream_iterator < std::string > (buffer)),
  std::istream_iterator < std::string > ());
  return ret;
}

vector < string > split (const string & str, const string & delim)
{
  vector < string > tokens;
  size_t
    prev = 0, pos = 0;
  do
    {
      pos = str.find (delim, prev);
      if (pos == string::npos)
  pos = str.length ();
      string
  token = str.substr (prev, pos - prev);
      if (!token.empty ())
  tokens.push_back (token);
      prev = pos + delim.length ();
    }
  while (pos < str.length () && prev < str.length ());
  return tokens;
}

/* The Configuration class is used to tell the bot how to connect
   to the appropriate exchange. The `test_exchange_index` variable
   only changes the Configuration when `test_mode` is set to `true`.
*/
class
  Configuration
{
private:
  /*
     0 = prod-like
     1 = slower
     2 = empty
   */
  static int const
    test_exchange_index = 0;
public:
  std::string
    team_name;
  std::string
    exchange_hostname;
  int
    exchange_port;
  Configuration (bool test_mode):
  team_name ("strawberry")
  {
    exchange_port = 20000;  /* Default text based port */
    if (true == test_mode)
      {
  exchange_hostname = "test-exch-" + team_name;
  exchange_port +=
    test_exchange_index;
      }
    else
      {
          exchange_hostname = "production";
      }
  }
};

/* Connection establishes a read/write connection to the exchange,
   and facilitates communication with it */
class
  Connection
{
private:
  FILE *
    in;
  FILE *
    out;
  int
    socket_fd;
public:
  Connection (Configuration configuration)
  {
    int
      sock = socket (AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
      {
  throw
  std::runtime_error ("Could not create socket");
      }
    std::string
      hostname = configuration.exchange_hostname;
    hostent *
      record = gethostbyname (hostname.c_str ());
    if (!record)
      {
  throw
    std::invalid_argument ("Could not resolve host '" + hostname + "'");
      }
    in_addr *
      address = reinterpret_cast < in_addr * >(record->h_addr);
    std::string ip_address = inet_ntoa (*address);
    struct sockaddr_in
      server;
    server.sin_addr.s_addr = inet_addr (ip_address.c_str ());
    server.sin_family = AF_INET;
    server.sin_port = htons (configuration.exchange_port);

    int
      res = connect (sock, ((struct sockaddr *) &server), sizeof (server));
    if (res < 0)
      {
  throw
  std::runtime_error ("could not connect");
      }
    FILE *
      exchange_in = fdopen (sock, "r");
    if (exchange_in == NULL)
      {
  throw
  std::runtime_error ("could not open socket for writing");
      }
    FILE *
      exchange_out = fdopen (sock, "w");
    if (exchange_out == NULL)
      {
  throw
  std::runtime_error ("could not open socket for reading");
      }

    setlinebuf (exchange_in);
    setlinebuf (exchange_out);
    this->in = exchange_in;
    this->out = exchange_out;
    this->socket_fd = res;
  }

  /** Send a string to the server */
  void
  send_to_exchange (std::string input)
  {
    std::string line (input);
    /* All messages must always be uppercase */
    std::transform (line.begin (), line.end (), line.begin (),::toupper);
    int
      res = fprintf (this->out, "%s\n", line.c_str ());
    if (res < 0)
      {
  throw
  std::runtime_error ("error sending to exchange");
      }
  }

  /** Read a line from the server, dropping the newline at the end */
  std::string read_from_exchange ()
  {
    /* We assume that no message from the exchange is longer
       than 10,000 chars */
    const
      size_t
      len = 10000;
    char
      buf[len];
    if (!fgets (buf, len, this->in))
      {
  throw
  std::runtime_error ("reading line from socket");
      }

    int
      read_length = strlen (buf);
    std::string result (buf);
    /* Chop off the newline */
    result.resize (result.length () - 1);
    return result;
  }
};

/** Join a vector of strings together, with a separator in-between
    each string. This is useful for space-separating things */
std::string join (std::string sep, std::vector < std::string > strs)
{
  std::ostringstream stream;
  const int size = strs.size ();
  for (int i = 0; i < size; ++i)
    {
      stream << strs[i];
      if (i != (strs.size () - 1))
  {
    stream << sep;
  }
    }
  return stream.str ();
}

void cancel_request(int id, Connection conn, unordered_map<int, PortfolioItem*>& id_to_outstanding) {
  string msg = "CANCEL " + to_string(id);
  conn.send_to_exchange(msg);

}

int symbol_to_int(string symbol) {
  int index = -1;

  if(symbol == "BOND") {index = 0;} 
  if(symbol == "GS") {index = 1;}
  if(symbol == "MS") {index = 2;}
  if(symbol == "USD") {index = 3;}
  if(symbol == "VALBZ") {index = 4;}
  if(symbol == "VALE") {index = 5;}
  if(symbol == "WFC") {index = 6;}
  if(symbol == "XLF") {index = 7;}

  return index;
}


void
convert (int &id, std::string symbol, string direction,
       int size, Connection conn, unordered_map<int, PortfolioItem*>& id_to_before_ack)
{

  id++;
  std::vector < std::string > request;
  request.push_back ("CONVERT");
  request.push_back (std::to_string (id));
  int id_copy = id;
  PortfolioItem* p = new PortfolioItem(symbol_to_int(symbol), direction =="SELL", id_copy, -1, size);
  p->convert = true;
  id_to_before_ack[id_copy] = p;

  request.push_back (symbol);
  request.push_back (direction);
  request.push_back (std::to_string (size));

  conn.send_to_exchange (join (" ", request));

}
void
add_request (int &id, std::string symbol, string direction, int price,
       int size, Connection conn, unordered_map<int, PortfolioItem*>& id_to_before_ack, vector<int>& outstanding_inventory)
{
  id++;

  std::vector < std::string > request;
  request.push_back ("ADD");
  request.push_back (std::to_string (id));


  int id_copy = id;
  PortfolioItem* p = new PortfolioItem(symbol_to_int(symbol), direction =="SELL", id_copy, price, size);
  id_to_before_ack[id_copy] = p;
  request.push_back (symbol);
  request.push_back (direction);
  request.push_back (std::to_string (price));
  request.push_back (std::to_string (size));
  if(direction == "SELL") {
    outstanding_inventory[symbol_to_int(symbol)] -= size;
  } else outstanding_inventory[symbol_to_int(symbol)] += size;

  conn.send_to_exchange (join (" ", request));
}

void
cancel_request (int &id, Connection conn)
{
  std::string request = "CANCEL " + std::to_string (id);
  conn.send_to_exchange (request);
}

void process_stock(vector<string> split_message, int sell_index, int& bond_sell, int& gs_sell, int& ms_sell, int& wfc_sell, vector<int> inventory, vector<int> outstanding_inventory, unordered_map<int, PortfolioItem*>& id_to_item_before_ack, int& global_id, Connection conn) {

      string symbol = split_message[1];
      int symbol_int = symbol_to_int(symbol);
      //for (int i = 3; i < sell_index; i++) {
      for(int i = 3; i < 4; i++) {
        vector < string > price_size = split (split_message[i], ":");
        int price = stoi (price_size[0]);
        int size = stoi (price_size[1]);

        if(symbol == "BOND") {
          if(bond_sell > 0) {
            add_request(global_id, "BOND", "SELL", price, min(bond_sell, size), conn, id_to_item_before_ack, outstanding_inventory);
            bond_sell -= min(bond_sell, size);
          }
        }


        if(symbol == "GS") {
          if(gs_sell > 0) {
            add_request(global_id, "GS", "SELL", price, min(gs_sell, size), conn, id_to_item_before_ack, outstanding_inventory);
            gs_sell -= min(gs_sell, size);
          }
        }


        if(symbol == "MS") {
          if(ms_sell > 0) {
            add_request(global_id, "MS", "SELL", price, min(ms_sell, size), conn, id_to_item_before_ack, outstanding_inventory);
            ms_sell -= min(ms_sell, size);
          }
        }


        if(symbol == "WFC") {
          if(wfc_sell > 0) {
            add_request(global_id, "WFC", "SELL", price, min(wfc_sell, size), conn, id_to_item_before_ack, outstanding_inventory);
            wfc_sell -= min(wfc_sell, size);
          }
        }

        }

      for (int i = sell_index + 1; i < sell_index+2; i++) {
//      for (int i = sell_index + 1; i < split_message.size(); i++) {
        if(i == split_message.size()) {break;}
        vector < string > price_size = split (split_message[i], ":");
        int price = stoi (price_size[0]);
        int size = stoi (price_size[1]);


        if(symbol == "BOND") {
          if(bond_sell < 0) {
            add_request(global_id, "BOND", "BUY", price, min(-bond_sell, size), conn, id_to_item_before_ack, outstanding_inventory);
            bond_sell += min(-bond_sell, size);
          }
        }


        if(symbol == "GS") {
          if(gs_sell < 0) {
            add_request(global_id, "GS", "BUY", price, min(-gs_sell, size), conn, id_to_item_before_ack, outstanding_inventory);
            gs_sell += min(-gs_sell, size);
          }
        }


        if(symbol == "MS") {
          if(ms_sell < 0) {
            add_request(global_id, "MS", "BUY", price, min(-ms_sell, size), conn, id_to_item_before_ack, outstanding_inventory);
            ms_sell += min(-ms_sell, size);
          }
        }


        if(symbol == "WFC") {
          if(wfc_sell < 0) {
            add_request(global_id, "WFC", "BUY", price, min(-wfc_sell, size), conn, id_to_item_before_ack, outstanding_inventory);
            wfc_sell += min(-wfc_sell, size);
          }
        }


      }
}

void
main_loop (Connection conn) {
  int global_id = rand()%1000000;

  const int BOND = 0;
  const int GS = 1;
  const int MS = 2;
  const int USD = 3;
  const int VALBZ = 4;
  const int VALE = 5;
  const int WFC = 6;
  const int XLF = 7;


  int VALBZ_avg = -1;
  int VALE_avg = -1;

  int GS_avg = -1;
  int MS_avg = -1;
  int WFC_avg = -1;
  int BOND_avg = -1;

  int bond_sell = 0;
  int gs_sell = 0;
  int ms_sell = 0;
  int wfc_sell = 0;

  int vale_sell = 0;

  
  int xlf_count = 0;

  std::vector <int> inventory(8);
  std::vector <int> outstanding_inventory(8);
  std::unordered_map < int, PortfolioItem* > id_to_item_before_ack; //id to PortfolioItem
  std::unordered_map < int, PortfolioItem* > id_to_item_outstanding; //id to PortfolioItem 

  while (true) {

  if(inventory[VALE] == 10) {
    convert(global_id, "VALBZ", "BUY", 10, conn, id_to_item_before_ack);
  }


  if(inventory[VALE] == -10) {
    cout << "AT -10" << endl;
    convert(global_id, "VALBZ", "SELL", 10, conn, id_to_item_before_ack);
  }

  if(inventory[XLF] > 50) {
    cout << "WE WILL TRY TO CONVERT " << endl;
    convert(global_id, "XLF", "SELL", 50, conn, id_to_item_before_ack);
  }

  if(inventory[XLF] < -50) {
    cout << "WE WILL TRY TO CONVERT " << endl;
    convert(global_id, "XLF", "BUY", 50, conn, id_to_item_before_ack);
  }


  if(xlf_count >= 10) {
    int num = xlf_count/10;
    xlf_count -= num*10;

    bond_sell += 3*num;
    gs_sell += 2*num;
    ms_sell += 3*num;
    wfc_sell += 2*num;
  } 
  if(xlf_count <= -10) {
    int num = xlf_count/10;
    xlf_count -= num*10;

    bond_sell += 3*num;
    gs_sell += 2*num;
    ms_sell += 3*num;
    wfc_sell += 2*num; 
  }


    for(const auto x: inventory) {cout << x << " ";}
    cout << endl;
    std::string message = conn.read_from_exchange ();
    vector < string > split_message = split (message, " ");
    if(split_message[0] == "ACK") {
      int id = stoi(split_message[1]);    
      if(id_to_item_before_ack.find(id) != id_to_item_before_ack.end()) {
        id_to_item_outstanding[id] = id_to_item_before_ack[id];
        PortfolioItem* item = id_to_item_before_ack[id];

        if(item->symbol == XLF && item->convert) {
          int size = item->size;
          if(item->direction) {
            inventory[XLF] -= size; 
            inventory[BOND] += 3*(size/10);
            inventory[GS] += 2*(size/10);
            inventory[MS] += 3*(size/10);
            inventory[WFC] += 2*(size/10);
          } else {
            inventory[XLF] += size; 
            inventory[BOND] -= 3*(size/10);
            inventory[GS] -= 2*(size/10);
            inventory[MS] -= 3*(size/10);
            inventory[WFC] -= 2*(size/10);
          }
          if(item->direction) {
            outstanding_inventory[item->symbol] += size;
          } else {
            outstanding_inventory[item->symbol] -= size;
          }
        }
      } else {
        cout << "ID_TO_ITEM DOES NOT CONTAIN THIS ID " << id << endl;
      }  
    }

    if(split_message[0] == "FILL") {
      int id = stoi(split_message[1]);
      string symbol = split_message[2];
      bool direction = split_message[3] == "SELL";
      int price = stoi(split_message[4]); 
      int size = stoi(split_message[5]);

      if(symbol == "XLF") {
        if(direction) {
          xlf_count -= size;
        } else {
          xlf_count += size;
        }
      }

      if(!direction) {
        inventory[symbol_to_int(symbol)] += size;
        outstanding_inventory[symbol_to_int(symbol)] -= size;
        inventory[symbol_to_int("USD")] -= size*price;
      } else {
        inventory[symbol_to_int(symbol)] -= size;
        outstanding_inventory[symbol_to_int(symbol)] += size;
        inventory[symbol_to_int("USD")] += size*price;
      }
      
      if(id_to_item_outstanding.find(id) != id_to_item_outstanding.end()) {  
        auto p = id_to_item_outstanding[id];
        p->decrease_size(size);
        cout << symbol_to_int(symbol) << " " << symbol << endl;
      }
    }

    if(split_message[0] == "OUT") {
      int id = stoi(split_message[1]);
      if(id_to_item_outstanding.find(id) != id_to_item_outstanding.end()) {
        id_to_item_outstanding.erase(id_to_item_outstanding.find(id));
      }
    }

    if(split_message[0] == "HELLO") {
      for(int i = 0; i < 8; i++) {
        auto item_num = split(split_message[i+1], ":");
        inventory[i] = stoi(item_num[1]);
      }
    }
    
    if (split_message[0] == "BOOK") {
      int sell_index = 0;
      for (int i = 0; i < split_message.size (); i++) {
        if (split_message[i] == "SELL") {
          sell_index = i;
          break;
        }
      }

      if(split_message[1] == "BOND" || split_message[1] == "GS" || split_message[1] == "MS" || split_message[1] == "WCF"){
      process_stock(split_message, sell_index, bond_sell, gs_sell, ms_sell, wfc_sell, inventory, outstanding_inventory, id_to_item_before_ack, global_id, conn);}


  if (split_message[1] == "GS") {
    int bid, offer;
    GS_avg = -1;
    bool buy_empty = split_message[3] == "SELL";
    if(!buy_empty) { 
      vector<string> buy_price_size = split(split_message[3], ":");
      int price = stoi(buy_price_size[0]);
      bid = price;
      int size = stoi(buy_price_size[1]);
    }

    bool sell_empty = split_message[split_message.size()-1] == "SELL";
    if(!sell_empty) {
      vector<string> buy_price_size = split(split_message[sell_index+1], ":");
      int price = stoi(buy_price_size[0]);
      offer = price;
      int size = stoi(buy_price_size[1]); 
    }

    if(!buy_empty && !sell_empty) {
      GS_avg = (bid+offer)/2;
    } else cout << "price not found" << endl;

  }


  if (split_message[1] == "MS") {
    int bid, offer;
    MS_avg = -1;
    bool buy_empty = split_message[3] == "SELL";
    if(!buy_empty) { 
      vector<string> buy_price_size = split(split_message[3], ":");
      int price = stoi(buy_price_size[0]);
      bid = price;
      int size = stoi(buy_price_size[1]);
    }

    bool sell_empty = split_message[split_message.size()-1] == "SELL";
    if(!sell_empty) {
      vector<string> buy_price_size = split(split_message[sell_index+1], ":");
      int price = stoi(buy_price_size[0]);
      offer = price;
      int size = stoi(buy_price_size[1]); 
    }

    if(!buy_empty && !sell_empty) {
      MS_avg = (bid+offer)/2;
    } else cout << "price not found" << endl;

  }


  if (split_message[1] == "WFC") {
    int bid, offer;
    WFC_avg = -1;
    bool buy_empty = split_message[3] == "SELL";
    if(!buy_empty) { 
      vector<string> buy_price_size = split(split_message[3], ":");
      int price = stoi(buy_price_size[0]);
      bid = price;
      int size = stoi(buy_price_size[1]);
    }

    bool sell_empty = split_message[split_message.size()-1] == "SELL";
    if(!sell_empty) {
      vector<string> buy_price_size = split(split_message[sell_index+1], ":");
      int price = stoi(buy_price_size[0]);
      offer = price;
      int size = stoi(buy_price_size[1]); 
    }

    if(!buy_empty && !sell_empty) {
      WFC_avg = (bid+offer)/2;
    } else cout << "price not found" << endl;

  }



  if (split_message[1] == "VALBZ") {
    int bid, offer;
    VALBZ_avg = -1;
    bool VALBZ_buy_empty = split_message[3] == "SELL";
    if(!VALBZ_buy_empty) { 
      vector<string> buy_price_size = split(split_message[3], ":");
      int price = stoi(buy_price_size[0]);
      bid = price;
      int size = stoi(buy_price_size[1]);
    }

    bool VALBZ_sell_empty = split_message[split_message.size()-1] == "SELL";
    if(!VALBZ_sell_empty) {
      vector<string> buy_price_size = split(split_message[sell_index+1], ":");
      int price = stoi(buy_price_size[0]);
      offer = price;
      int size = stoi(buy_price_size[1]); 
    }

    if(!VALBZ_buy_empty && !VALBZ_sell_empty) {
      VALBZ_avg = (bid+offer)/2;
    } else cout << "price not found" << endl;
  }


  if (split_message[1] == "XLF") {

    
    if(GS_avg == -1 || MS_avg == -1 || WFC_avg == -1) {continue;}
    int XLF_estimate = (3000 + 2*GS_avg + 3*MS_avg + 2*WFC_avg)/10;
    for (int i = 3; i < sell_index; i++) {
      vector<string> price_size = split(split_message[i], ":");

      int price = stoi(price_size[0]);
      int size = stoi(price_size[1]);

      int spread = 20;
     
/*
      if(inventory[XLF] < -50) {spread = 20;}
      if(inventory[XLF] < -75) {spread = 30;}
      if(inventory[XLF] < -90) {spread = 50;}*/

      if (price > XLF_estimate + spread) {
        add_request(global_id, "XLF", "SELL", price, size, conn, id_to_item_before_ack, outstanding_inventory);
        cout << " SELL XLF" << "\n";
      }
    }
    for (int i = sell_index + 1; i < split_message.size(); i++) {
      vector<string> price_size = split(split_message[i], ":");
      int price = stoi(price_size[0]);
      int size = stoi(price_size[1]);
      int spread = 10;
/*
      if(inventory[XLF] > 50) {spread = 20;}
      if(inventory[XLF] > 75) {spread = 30;}
      if(inventory[XLF] > 90) {spread = 50;}*/
      if (price < XLF_estimate - spread) {

        add_request(global_id, "XLF", "BUY", price, size, conn, id_to_item_before_ack, outstanding_inventory);
        cout << " BUY XLF" << " " << price << " " << size << endl;
      }
    }
  }
  
  if (split_message[1] == "VALE") {

    for (int i = 3; i < sell_index; i++) {
      vector<string> price_size = split(split_message[i], ":");
      int price = stoi(price_size[0]);
      int size = stoi(price_size[1]);

      int spread = 10;
      if (VALBZ_avg != -1 && price > VALBZ_avg + spread) {
        add_request(global_id, "VALE", "SELL", price, size, conn, id_to_item_before_ack,outstanding_inventory);
        add_request(global_id, "VALBZ", "BUY", price, size, conn, id_to_item_before_ack, outstanding_inventory);
      }
    }
    for (int i = sell_index + 1; i < split_message.size(); i++) {
      vector<string> price_size = split(split_message[i], ":");
      int price = stoi(price_size[0]);
      int size = stoi(price_size[1]);
      int spread = 10;
      if (VALBZ_avg != -1 && price < VALBZ_avg - spread) {
        add_request(global_id, "VALE", "BUY", price, size, conn, id_to_item_before_ack, outstanding_inventory);
        add_request(global_id, "VALBZ", "SELL", price, size, conn, id_to_item_before_ack, outstanding_inventory);
        cout << " BUY VALE" << " " << price << " " << size << endl;
      }
    }
  }
 
    }



  }


}



int
main (int argc, char *argv[])
{
  // Be very careful with this boolean! It switches between test and prod
  bool test_mode = false;
  Configuration config (test_mode);
  Connection conn (config);

  std::vector < std::string > data;
  data.push_back (std::string ("HELLO"));
  data.push_back (config.team_name);
  /*
     A common mistake people make is to conn.send_to_exchange() > 1
     time for every conn.read_from_exchange() response.
     Since many write messages generate marketdata, this will cause an
     exponential explosion in pending messages. Please, don't do that!
   */
  conn.send_to_exchange (join (" ", data));
  main_loop (conn);


  return 0;
}
