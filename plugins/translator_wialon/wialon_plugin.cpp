#include "wialon_plugin.hpp"

#include <boost/container/vector.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>

#include <boost/assign/list_of.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/typeof/std/utility.hpp>
#include <boost/make_shared.hpp>

#include <boost/algorithm/string/trim.hpp>

//#define unexpected_token(expected, got) parser_error("Unexpected token, expected: '" + expected +"' got: '" + got + "'");

#pragma push_macro("_")
#undef _
#define _(x, y) boost::make_tuple<std::size_t, command_t>(x, y)
wialon_plugin::estimates_t wialon_plugin::estimates_ = boost::assign::map_list_of
  ("L",  _(2,  COMMAND_LOGIN))
  ("D",  _(16, COMMAND_DATA))
  ("P",  _(0,  COMMAND_PING))
  ("SD", _(10, COMMAND_SHORTDATA))
  ("B",  _(16, COMMAND_BLACKBOX))
  ("M",  _(1,  COMMAND_MESSAGE))
  ("I",  _(6,  COMMAND_IMAGE))
;
#undef _
#define _(x) boost::assign::list_of(x)
wialon_plugin::fields_t wialon_plugin::fields_ = boost::assign::list_of<fields_subtype>
/*
  //boost::adaptors::reverse
  (_("imei")("password"))
  (_("date")("time")("lat1")("lat2")("lon1")("lon2")
    ("speed")("course")("height")("sats")("hdop")
    ("inputs")("outputs")("adc")("ibutton")("params"))
  ()
  (_("date")("time")("lat1")("lat2")("lon1")("lon2")
    ("speed")("course")("height")("sats"))
  ()
  (_("message"))
  (_("sz")("ind")("count")("date")("time")("name"))
*/
  (_("password")("imei"))
  (_("params")("ibutton")("adc")("outputs")("inputs")
    ("hdop")("sats")("height")("course")("speed")
    ("lon2")("lon1")("lat2")("lat1")("time")("date"))
  (/* EMPTY */)
  (_("sats")("height")("course")("speed")
    ("lon2")("lon1")("lat2")("lat1")("time")("date"))
  (_("params")("ibutton")("adc")("outputs")("inputs")
    ("hdop")("sats")("height")("course")("speed")
    ("lon2")("lon1")("lat2")("lat1")("time")("date"))
  (_("message"))
  (_("name")("time")("date")("count")("ind")("sz"))
;
#pragma pop_macro("_")

wialon_plugin::wialon_plugin()
{
  std::cout << name() << " created" << std::endl;

  authenticated_ = false;
  state_ = STATE_INITIAL;
}

wialon_plugin::~wialon_plugin()
{
  std::cout << name() << " destroyed" << std::endl;
}

void wialon_plugin::handle_start()
{
  api_->do_read_until(sbuf_, "\r\n");
}

void wialon_plugin::handle_read(std::size_t bytes_transferred)
{
  //std::cout << "*handle_read*";
  //api_->do_write(boost::asio::buffer(buffer_, bytes_transferred));
  //std::cout << std::string(buffer_.c_array()).substr(0, bytes_transferred) << std::endl;


  /*std::string msg = buffer_.data();*/

  std::string msg;
  std::istream is(&sbuf_);


  while (true) {
    if (!sbuf_.size()) {
      std::cout << "[ALEERT SOMTHING BAAD]" << std::endl;
      return;
    }

    std::string tmp;

    std::getline(is, tmp);
    sbuf_.consume(tmp.size() + 1); /// getline() consumes LF but not CR

    BOOST_AUTO(it, tmp.end());
    --it;
    if ('\r' == *it) {
      msg.append(tmp.begin(), it);
      break;
    }
    else {
      msg.append(tmp).append("\n"); /// NOTE: Hack for consumed LF
    }
  }

  std::cout << "input string: " << boost::property_tree::json_parser::create_escapes(msg) << "\n"
            << "size: " << msg.size() << std::endl;

  parse_string(msg);
  consume_token("\r\n"); /// NOTE: Hack for consumed LF

  if (state_ != STATE_BINARY_DATA) {
    api_->do_read_until(sbuf_, "\r\n");
  }
  else {

  }
}

void wialon_plugin::handle_write()
{
  //std::cout << "*handle_write*";
  //api_->do_read(boost::asio::buffer(buffer_));
}

void wialon_plugin::parse_string(const std::string& msg)
{
  static boost::char_separator<char> sep_(";", "#|", boost::keep_empty_tokens);

  boost::tokenizer<boost::char_separator<char> > tokens(msg, sep_);
  BOOST_FOREACH(const std::string& tok, tokens) {
    std::cout << "comsume_token(): "
              << tok.size()
              << ": \""
              << boost::property_tree::json_parser::create_escapes(tok)
              << "\" "
              /*<< std::hex << std::setw(2) << std::setfill('0') << tok*/
              /*<< (boost::format("%X") % tok)*/
              /*<< hex_cast::hex_as<std::string>(tok)*/
              << std::endl;
    consume_token(tok);
  }
}

void wialon_plugin::consume_token(const std::string& tok)
{
  if ("\r\n" == tok) {
    switch (state_) {
    case STATE_BODY: {
      if (current_cmd_ != COMMAND_BLACKBOX) {
        store_data();
        parser_error("Unexpected end of body (still remaining " + boost::lexical_cast<std::string>(estimate_) + " parameters)");
        break;
      }
      /// Fall through
    case STATE_BAR:
      store_data();
      ++multicommand_;
      /// Fall through
    case STATE_SKIP_TO_BAR_OR_END:
      commit_command();
      break;

    case STATE_COMMAND:
      if (/*tcp == protocol &&*/ !authenticated_) {
        unexpected_token("L", tok);
      }
      else {
        unexpected_token("B|D|I|M|P|SD", tok);
      }
      break;

    case STATE_HASH_FIRST:
    case STATE_HASH_SECOND:
      unexpected_token("#", tok);
      break;

    case STATE_END:
      commit_command();
      break;
    }
    }
    state_ = STATE_INITIAL;
    return;
  }

  switch (state_) {
  case STATE_INITIAL:
    //tree_.reset(boost::make_shared<boost::property_tree::ptree>());
    boost::make_shared<boost::property_tree::ptree>().swap(tree_);
    multicommand_ = 0;
    imei_ = "unknown";
    cmd_ = "not initialized";
    /// Fall through
  case STATE_IMEI:
    /// TODO: Can imei be changed during session?
    /*if (udp == protocol && !authenticated_) {
      /// TODO: Call API to set connection guid
      imei_ = tok;
      authenticated_ = true;
    }*/
    std::cout << boost::lexical_cast<std::string>(state_) << " changed to STATE_HASH" << std::endl;
    state_ = STATE_HASH_FIRST;
    break;

  case STATE_COMMAND:
    cmd_ = tok;
    {
    BOOST_AUTO(const it, estimates_.find(tok));
    if (it == estimates_.end()) {
      state_ = STATE_SKIP_TO_END;
      parser_error("Unknown command: '" + tok + "'");
      break;
    }

    boost::tie(estimate_, current_cmd_) = it->second;
    }

    if (!authenticated_ && current_cmd_ != COMMAND_LOGIN) {
      state_ = STATE_SKIP_TO_END;
      std::cout << "STATE_COMMAND changed to STATE_SKIP_TO_END due to !authenticated_" << std::endl;
      answer("not too faaaaaast"); /// FIXME: REMOVE ME
      break;
    }

    state_ = STATE_HASH_SECOND;
    std::cout << "STATE_COMMAND changed to STATE_HASH "<< boost::lexical_cast<std::string>(current_cmd_) << std::endl;
    break;

  case STATE_BODY:
    if (current_cmd_ == COMMAND_BLACKBOX  && "|" == tok) {
      store_data();
      ++multicommand_;
      break;
    }

    if (estimate_) {
      fields_type field_name = fields_[current_cmd_][--estimate_];
      tree_->put(field_name, tok);
    }

    if (!estimate_) {
      state_ = ( (current_cmd_ != COMMAND_BLACKBOX) ? STATE_END : STATE_BAR );
      std::cout << "STATE_BODY changed to " << boost::lexical_cast<std::string>(state_) << std::endl;
    }
    else {
      std::cout << "STATE_BODY unchanged" << std::endl;
    }
    break;

  case STATE_HASH_FIRST:
  case STATE_HASH_SECOND:
    if ("#" == tok) {
      const std::string prev_state = boost::lexical_cast<std::string>(state_);
      state_ = ( (state_ == STATE_HASH_FIRST) ? STATE_COMMAND : STATE_BODY );
      std::cout << prev_state << " change to " << boost::lexical_cast<std::string>(state_) << std::endl;
      break;
    }
    state_ = STATE_SKIP_TO_END;
    unexpected_token("#", tok);
    break;

  case STATE_BAR:
    state_ = STATE_BODY;
    if ("|" != tok) {
      state_ = STATE_SKIP_TO_BAR_OR_END;
      unexpected_token("|", tok);
    }
    store_data();
    ++multicommand_;
    break;

  case STATE_SKIP_TO_BAR_OR_END:
    if ("|" == tok) {
      state_ = STATE_BODY;
    }
    break;

  case STATE_END:
    state_ = STATE_SKIP_TO_END;
    unexpected_token("\\r\\n", tok);
    break;
  }
}

void wialon_plugin::unexpected_token(const std::string& expected,
                                     const std::string& got)
{
  parser_error("Unexpected token, expected: '" + boost::property_tree::json_parser::create_escapes(expected) +"' got: '" + boost::property_tree::json_parser::create_escapes(got) + "'");
}

void wialon_plugin::parser_error(const std::string& message)
{
  std::cout << "[Parser::Error] " << message << std::endl;
}

void wialon_plugin::commit_command()
{
  std::stringstream ss;
  boost::property_tree::json_parser::write_json(ss, *tree_, true);
  std::cout << "commit_command(): " << cmd_ << "\n" << ss.str() << std::endl;

  switch (current_cmd_) {
  case COMMAND_LOGIN:
    std::cout << "Logining with " << tree_->get<std::string>("imei") << ":" << tree_->get<std::string>("password") << std::endl;
    answer("1");authenticated_ = true; // FIXME: REMOVE ME
    /// TODO: Check for brute-force attack!?
    /*if (parse error) {
      answer("0");
    }
    else {
      api->check_credentials(
          tree_->get<std::string>("imei"),
          tree_->get<std::string>("password"),
          boost::bind(&wialon_plugin::handle_check_credentials,
                      weak_ptr(shared_from_this()), _1))
      imei_ = tok;
    }*/
    break;
  case COMMAND_SHORTDATA:
  case COMMAND_DATA:
    //if (ok) answer("1")
    break;
  case COMMAND_PING:
    answer("");
    break;
  case COMMAND_MESSAGE:
    break;
  case COMMAND_BLACKBOX:
    answer(boost::lexical_cast<std::string>(multicommand_));
    break;
  case COMMAND_IMAGE:
    state_ = STATE_BINARY_DATA;
    /// TODO: Check size for safety
    estimate_ = tree_->get<std::size_t>("sz");
    /// Read binary and then set to STATE_INITIAL
    /// delayed answer
    break;
  }
}

void wialon_plugin::store_data()
{
  commit_command();
}

void wialon_plugin::answer(const std::string& msg)
{
  /// TODO: Use Boost.Karma for this
  std::string reply = "#A";
  reply.append(cmd_).append("#").append(msg);
  std::copy(reply.begin(), reply.end(), answer_buffer_.begin());

  api_->do_write(boost::asio::buffer(answer_buffer_));
}

/*void wialon_plugin::handle_check_credentials(bool ok)
{
  if (ok) {
    authenticated_ = true;
    answer("1");
  }
  else {
    answer("01");
    /// TODO: Close connection?
  }
}*/
