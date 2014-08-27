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

namespace eiptnd {

using boost::property_tree::json_parser::create_escapes;

template <typename CharT, typename TraitsT>
inline std::basic_ostream<CharT, TraitsT>& operator<< (
  std::basic_ostream<CharT, TraitsT>& os, const wialon_plugin::state_t state)
{
  static const char* strings[] = {
    "STATE_INITIAL",
    "STATE_IMEI",
    "STATE_COMMAND",
    "STATE_BODY",
    "STATE_HASH_FIRST",
    "STATE_HASH_SECOND",
    "STATE_BAR",
    "STATE_END",
    "STATE_SKIP_TO_BAR_OR_END",
    "STATE_SKIP_TO_END",
    "STATE_BINARY_DATA"
  };

  return os << strings[state];
}

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
  : log_(boost::log::keywords::channel = "plugin2")
  , authenticate_callback_(boost::bind(&wialon_plugin::handle_authenticate, this, _1))
  , process_data_callback_(boost::bind(&wialon_plugin::handle_process_data, this, _1))
{
  BOOST_LOG_SEV(log_, logging::trace) << name() << " created";

  authenticated_ = false;
  state_ = STATE_INITIAL;
}

wialon_plugin::~wialon_plugin()
{
  BOOST_LOG_SEV(log_, logging::trace) << name() << " destroyed";

  boost::log::core::get()->flush(); /// FIXME: At reworking logging
}

void wialon_plugin::handle_start()
{
  api_->do_read_until(sbuf_, "\r\n");
}

void wialon_plugin::handle_read(std::size_t bytes_transferred)
{
  std::string msg;
  std::istream is(&sbuf_);

  while (true) {
    if (!sbuf_.size()) {
      BOOST_LOG_SEV(log_, logging::error) << "Happend something bad";
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

  BOOST_LOG_SEV(log_, logging::trace)
    << "[Parser::Input] size: " << msg.size()
    << "data: " << create_escapes(msg);

  parse_string(msg);
  consume_token("\r\n"); /// NOTE: Hack for consumed LF

  if (state_ != STATE_BINARY_DATA) {
    api_->do_read_until(sbuf_, "\r\n");
  }
  else {
    /// Read (estimate_ - sbuf_.size()) bytes
  }
}

void wialon_plugin::handle_write()
{
}

void wialon_plugin::parse_string(const std::string& msg)
{
  static boost::char_separator<char> sep_(";", "#|", boost::keep_empty_tokens);

  boost::tokenizer<boost::char_separator<char> > tokens(msg, sep_);
  BOOST_FOREACH(const std::string& tok, tokens) {
    BOOST_LOG_SEV(log_, logging::trace)
      << "[Parser::Consume] size: " << tok.size()
      << " token: " << create_escapes(tok);

    consume_token(tok);
  }
}

void wialon_plugin::consume_token(const std::string& tok)
{
  if ("\r\n" == tok) {
    switch (state_) {
    case STATE_BODY: {
      if (current_cmd_ != COMMAND_BLACKBOX) {
        parser_error("Unexpected end of body (still remaining " + boost::lexical_cast<std::string>(estimate_) + " parameters)");
        commit_command();
        break;
      }
      /// Fall through
    case STATE_BAR:
      store_data();
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
    is_parser_error_ = false;
    multicommand_ = 0;
    cmd_ = "<not initialized>";
    /// Fall through
  case STATE_IMEI:
    /// TODO: Can imei be changed during session?
    /*if (udp == protocol && !authenticated_) {
      /// TODO: Call API to set connection guid
      authenticated_ = true;
    }*/
    BOOST_LOG_SEV(log_, logging::trace)
      << "[Parser::State] " << state_
      << " changed to STATE_HASH_FIRST";
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
      BOOST_LOG_SEV(log_, logging::trace)
        << "[Parser::State] STATE_COMMAND change to STATE_SKIP_TO_END"
           " due to not authenticated";
      break;
    }

    state_ = STATE_HASH_SECOND;
    BOOST_LOG_SEV(log_, logging::trace)
      << "[Parser::State] STATE_COMMAND change to " << state_
      << " by command " << current_cmd_;
    break;

  case STATE_BODY:
    if (current_cmd_ == COMMAND_BLACKBOX  && "|" == tok) {
      store_data();
      boost::tie(estimate_, current_cmd_) = estimates_.at(cmd_);
      boost::make_shared<boost::property_tree::ptree>().swap(tree_);
      break;
    }

    if (estimate_) {
      fields_type field_name = fields_[current_cmd_][--estimate_];
      tree_->put(field_name, tok);
    }

    if (!estimate_) {
      state_ = ( (current_cmd_ != COMMAND_BLACKBOX) ? STATE_END : STATE_BAR );
      BOOST_LOG_SEV(log_, logging::trace)
        << "[Parser::State] STATE_BODY changed to " << state_;
    }
    else {
      BOOST_LOG_SEV(log_, logging::trace)
        << "[Parser::State] STATE_BODY unchanged";
    }
    break;

  case STATE_HASH_FIRST:
  case STATE_HASH_SECOND:
    if ("#" == tok) {
      const state_t prev_state = state_;
      state_ = ( (state_ == STATE_HASH_FIRST) ? STATE_COMMAND : STATE_BODY );
      BOOST_LOG_SEV(log_, logging::trace)
        << "[Parser::State] " << prev_state << " changed to " << state_;
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
  parser_error("Unexpected token, expected: '" + create_escapes(expected) +"' got: '" + create_escapes(got) + "'");
}

void wialon_plugin::parser_error(const std::string& message)
{
  BOOST_LOG_SEV(log_, logging::trace) << "[Parser::Error] " << message;

  is_parser_error_ = true;
}

void wialon_plugin::commit_command()
{
  std::stringstream ss;
  boost::property_tree::json_parser::write_json(ss, *tree_, true);
  BOOST_LOG_SEV(log_, logging::trace)
    << "commit_command(): " << cmd_ << "\n" << ss.str();

  switch (current_cmd_) {
  case COMMAND_LOGIN:
    if (is_parser_error_) {
      answer("0");
    }
    else {
      api_->authenticate(
          tree_->get<std::string>("imei"),
          tree_->get<std::string>("password"),
          authenticate_callback_);
    }
    break;

  case COMMAND_SHORTDATA:
  case COMMAND_DATA:
    store_data();
    break;

  case COMMAND_PING:
    answer("");
    break;

  case COMMAND_MESSAGE:
    store_data();
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
  api_->process_data(tree_, process_data_callback_);
}

void wialon_plugin::answer(const std::string& msg)
{
  /// TODO: Use Boost.Karma for this
  std::string reply = "#A";
  reply.append(cmd_).append("#").append(msg);
  std::copy(reply.begin(), reply.end(), answer_buffer_.begin());

  BOOST_LOG_SEV(log_, logging::trace) << "Responding with reply: " << reply;

  api_->do_write(boost::asio::buffer(answer_buffer_));
}

void wialon_plugin::handle_authenticate(bool ok)
{
  state_ = STATE_INITIAL;
  if (ok) {
    BOOST_LOG_SEV(log_, logging::trace) << "Authentication is successful";
    authenticated_ = true;
    answer("1");
  }
  else {
    BOOST_LOG_SEV(log_, logging::trace) << "Authentication is rejected";
    answer("01");
    /// TODO: Close connection?
  }
}

void wialon_plugin::handle_process_data(bool ok)
{
  if (current_cmd_ == COMMAND_BLACKBOX) {
    /// TODO: Should we check for is ok?
    ++multicommand_;
  }
  else if (ok) {
    /// TODO: It can be other because of is_parser_error_
    answer("1");
  }
  else {
    if (current_cmd_ == COMMAND_DATA || current_cmd_ == COMMAND_SHORTDATA) {
      answer("-1");
    }
    else {
      answer("0");
    }
  }
}

} // namespace eiptnd
