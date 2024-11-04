#include "peaclock/peaclock.hh"

#include "ob/rect.hh"
using Rect = OB::Rect;

#include "ob/string.hh"
#include "ob/text.hh"
#include "ob/term.hh"
namespace aec = OB::Term::ANSI_Escape_Codes;

#include <ctime>
#include <cstddef>
#include <cstdlib>

#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <algorithm>

void Peaclock::init_ctx(std::size_t const width, std::size_t const height)
{
  _ctx = Ctx();

  _ctx.width = width;
  _ctx.height = height;

  set_clock_value();

  // cfg.style.title.step();
  cfg.style.date.step();
  cfg.style.background.step();
  cfg.style.active_fg.step();
  cfg.style.inactive_fg.step();
  cfg.style.colon_fg.step();
  cfg.style.active_bg.step();
  cfg.style.inactive_bg.step();
  cfg.style.colon_bg.step();

  if (cfg.view != View::date && cfg.view != View::ascii)
  {
    // --_ctx.height;

    // set the buffer to a template
    if (cfg.view == View::digital)
    {
      _ctx.buffer = cfg.seconds ? _clock_digital_seconds : _clock_digital;
    }
    else if (cfg.view == View::binary)
    {
      _ctx.buffer = cfg.seconds ? _clock_binary_seconds : _clock_binary;
    }
    else
    {
      _ctx.buffer = _icon;
    }

    _ctx.x_blocks = count_x_blocks(_ctx.buffer);
    _ctx.y_blocks = count_y_blocks(_ctx.buffer);

    _ctx.x_spaces = _ctx.x_blocks - 1;
    _ctx.y_spaces = _ctx.y_blocks - 1;

    calc_xy_block();
    calc_xy_ratio();
    calc_xy_begin();

    // init block
    _ctx.block.xy_max(_ctx.width + 1, _ctx.height + 1);
    _ctx.block.wh(_ctx.x_block, _ctx.y_block);

    // init fill
    _ctx.fill_active = set_fill(cfg.fill_active);
    _ctx.fill_inactive = set_fill(cfg.fill_inactive);
    _ctx.fill_colon = set_fill(cfg.fill_colon);

    // set starting coordinates
    _ctx.x = _ctx.x_begin;
    _ctx.y = _ctx.y_begin;
  }
}

// void Peaclock::draw_background(std::size_t const width, std::size_t const height, std::ostringstream& buf)
// {
//   _ctx.background.xy_max(_ctx.width + 1, _ctx.height + 1);
//   _ctx.background.xy(0, 0);
//   _ctx.background.wh(_ctx.width, _ctx.height);
//   _ctx.background.color_bg(cfg.style.background);

//   buf << _ctx.background;
// }

// void Peaclock::draw_title(std::ostringstream& buf)
// {
//   if (cfg.title && cfg.titlefmt.size())
//   {
//     _ctx.text.xy_max(_ctx.width + 1, _ctx.height + 1);
//     _ctx.text.xy(0, _ctx.y);
//     _ctx.text.wh(_ctx.width, cfg.height_titlefmt);
//     _ctx.text.color_fg(cfg.style.title);
//     _ctx.text.color_bg(cfg.style.background);
//     _ctx.text.text(cfg.titlefmt);
//     _ctx.text.align(Rect::Align::center, Rect::Align::top);

//     buf << _ctx.text;

//     _ctx.y += 1 + cfg.height_titlefmt;
//   }
// }

void Peaclock::draw_clock(std::ostringstream& buf)
{

  _inverse = !_inverse;

  if (cfg.view == View::date)
  {
    return;
  }
  else if (cfg.view == View::ascii)
  {
    draw_ascii(buf);
  }
  else // if (cfg.view == View::clock)
  {
    if (cfg.view == View::digital)
    {
      set_clock_digital();
    }
    else if (cfg.view == View::binary)
    {
      set_clock_binary();
    }

    // draw blocks
    for (std::size_t i = 0; i < _ctx.buffer.size(); ++i)
    {
      auto type = _ctx.buffer.at(i);

      if (_inverse) {
        if ((cfg.mode == Mode::timer && !timer) || (cfg.mode == Mode::stopwatch && !stopwatch)) {
          if (type == Type::on)
            type = Type::off;
        }
      }

      switch (type)
      {
        case Type::empty:
        {
          _ctx.x += _ctx.x_block + cfg.x_space.get();

          break;
        }

        case Type::off:
        {
          _ctx.block.xy(_ctx.x, _ctx.y);
          _ctx.block.text("");
          _ctx.block.align(Rect::Align::center, Rect::Align::center);

          _ctx.block.color_fg(cfg.style.inactive_fg);
          _ctx.block.color_bg(
            cfg.style.inactive_bg.mode() == OB::Color::Mode::party ||
            cfg.style.inactive_bg.mode() == OB::Color::Mode::candy ?
            cfg.style.inactive_bg.step() : cfg.style.inactive_bg.key() == "clear" ?
            cfg.style.background : cfg.style.inactive_bg);

          if (! cfg.fill_inactive.empty())
          {
            _ctx.block.text(_ctx.fill_inactive);
          }

          buf << _ctx.block;

          _ctx.x += _ctx.x_block + cfg.x_space.get();

          break;
        }

        case Type::on:
        {
          _ctx.block.xy(_ctx.x, _ctx.y);
          _ctx.block.text("");
          _ctx.block.align(Rect::Align::center, Rect::Align::center);

          _ctx.block.color_fg(cfg.style.active_fg);
          _ctx.block.color_bg(
            cfg.style.active_bg.mode() == OB::Color::Mode::party ||
            cfg.style.active_bg.mode() == OB::Color::Mode::candy ?
            cfg.style.active_bg.step() : cfg.style.active_bg.key() == "clear" ?
            cfg.style.background : cfg.style.active_bg);

          if (! cfg.fill_active.empty())
          {
            _ctx.block.text(_ctx.fill_active);
          }

          buf << _ctx.block;

          _ctx.x += _ctx.x_block + cfg.x_space.get();

          break;
        }

        case Type::colon:
        {
          _ctx.block.xy(_ctx.x, _ctx.y);
          _ctx.block.text("");
          _ctx.block.align(Rect::Align::center, Rect::Align::center);

          _ctx.block.color_fg(cfg.style.colon_fg);
          _ctx.block.color_bg(
            cfg.style.colon_bg.mode() == OB::Color::Mode::party ||
            cfg.style.colon_bg.mode() == OB::Color::Mode::candy ?
            cfg.style.colon_bg.step() : cfg.style.colon_bg.key() == "clear" ?
            cfg.style.active_bg : cfg.style.colon_bg);

          if (! cfg.fill_colon.empty())
          {
            _ctx.block.text(_ctx.fill_colon);
          }

          buf << _ctx.block;

          _ctx.x += _ctx.x_block + cfg.x_space.get();

          break;
        }

        case Type::newline:
        {
          _ctx.x = _ctx.x_begin;
          _ctx.y += _ctx.y_block + cfg.y_space.get();

          break;
        }

        default:
        {
          break;
        }
      }
    }
  }
}

void Peaclock::draw_ascii(std::ostringstream& buf)
{
  std::size_t const y = ([&]() {
    return (_ctx.height / 2) - ((1 + (cfg.date && cfg.height_datefmt ? cfg.height_datefmt + cfg.date_padding.get() : 0)) / 2);
  })();
  _ctx.y += y;

  _ctx.text.xy_max(_ctx.width + 1, _ctx.height + 1);
  _ctx.text.xy(0, y);
  _ctx.text.wh(_ctx.width, 1);
  _ctx.text.color_fg(cfg.style.active_fg);
  _ctx.text.color_bg(cfg.style.background);
  _ctx.text.text(std::to_string(_ctx.value.at(0)) + std::to_string(_ctx.value.at(1)) + ":" + std::to_string(_ctx.value.at(2)) + std::to_string(_ctx.value.at(3)) +
    (cfg.seconds ? ":" + std::to_string(_ctx.value.at(4)) + std::to_string(_ctx.value.at(5)) : ""));
  _ctx.text.align(Rect::Align::center, Rect::Align::center);
  _ctx.text.highlight({{":", cfg.style.colon_fg}});

  buf << _ctx.text;
}

void Peaclock::draw_date(std::ostringstream& buf)
{
  if (cfg.date && cfg.datefmt.size())
  {
    std::size_t const y = ([&]() {
      if (cfg.view == View::date)
      {
        return _ctx.height / 2;
      }

      if (cfg.view == View::ascii)
      {
        return _ctx.y + cfg.date_padding.get() + 1;
      }

      return _ctx.y + cfg.date_padding.get() - cfg.y_space.get();
    })();

    _ctx.text.xy_max(_ctx.width + 1, _ctx.height + 1);
    _ctx.text.xy(0, y);
    _ctx.text.wh(_ctx.width, cfg.height_datefmt);
    _ctx.text.color_fg(cfg.style.date);
    _ctx.text.color_bg(cfg.style.background);
    _ctx.text.text(_ctx.datefmt);
    _ctx.text.align(Rect::Align::center, Rect::Align::top);
    _ctx.text.highlight({{":", cfg.style.colon_fg}});

    buf << _ctx.text;
  }
}

void Peaclock::render(std::size_t const width, std::size_t const height, std::ostringstream& buf)
{
  init_ctx(width, height);

  // draw_background(width, height, buf);
  // draw_date(buf);
  draw_clock(buf);
  draw_date(buf);
}

std::size_t Peaclock::find(Peaclock::Clock const& vec, int const val, std::size_t const pos) const
{
  auto const res = std::find(vec.begin() + static_cast<long int>(pos), vec.end(), val);

  return res != vec.end() ? static_cast<std::size_t>(std::distance(vec.begin(), res)) : npos;
}

std::size_t Peaclock::count(Clock const& vec, int const val) const
{
  std::size_t pos {0};
  std::size_t count {0};

  for (;;)
  {
    pos = find(vec, val, pos);

    if (pos == npos)
    {
      break;
    }

    ++count;
    ++pos;
  }

  return count;
}

std::size_t Peaclock::count_x_blocks(Clock const& clock) const
{
  auto pos = find(clock, 2);

  if (pos == npos)
  {
    pos = clock.size();
  }

  std::size_t count {0};

  for (std::size_t i = 0; i < pos; ++i)
  {
    if (clock.at(i) == -1 || clock.at(i) == 0 || clock.at(i) == 1)
    {
      ++count;
    }
  }

  return count;
}

std::size_t Peaclock::count_y_blocks(Clock const& clock) const
{
  return count(clock, 2);
}

void Peaclock::extract_digits(int const num, int& t0, int& t1) const
{
  if (num < 10)
  {
    t0 = 0;
    t1 = num;
  }
  else
  {
    t0 = num / 10;
    t1 = num % 10;
  }
}

std::string Peaclock::set_fill(std::string const& fill) const
{
  auto ln = OB::String::repeat(_ctx.x_block, fill);
  OB::Text::View lnv;
  lnv.str(ln);
  return OB::String::repeat(_ctx.y_block, std::string(lnv.colstr(0, _ctx.x_block)) + "\n");
}

void Peaclock::calc_xy_block()
{
  if (auto const val = (_ctx.width - (cfg.x_space.get() * _ctx.x_spaces) -
    (cfg.x_border.get() * 2)) / _ctx.x_blocks;
    static_cast<int>(val) > 0)
  {
    _ctx.x_block = val;
  }
  else
  {
    _ctx.x_block = 1;
  }

  if (! cfg.auto_size && (_ctx.x_block > cfg.x_block.get()))
  {
    _ctx.x_block = cfg.x_block.get();
  }

  if (auto const val = (_ctx.height - (cfg.y_space.get() * _ctx.y_spaces) - (cfg.y_border.get() * 2) -
    // (cfg.title && cfg.height_titlefmt ? cfg.height_titlefmt + 1 : 0) -
    (cfg.date && cfg.height_datefmt ? cfg.height_datefmt + cfg.date_padding.get() : 0)) / _ctx.y_blocks;
    static_cast<int>(val) > 0)
  {
    _ctx.y_block = val;
  }
  else
  {
    _ctx.y_block = 1;
  }

  if (! cfg.auto_size && (_ctx.y_block > cfg.y_block.get()))
  {
    _ctx.y_block = cfg.y_block.get();
  }
}

void Peaclock::calc_xy_ratio()
{
  if (cfg.auto_ratio)
  {
    if (_ctx.x_block > cfg.x_ratio.get())
    {
      _ctx.x_block -= _ctx.x_block % cfg.x_ratio.get();
    }

    if (_ctx.y_block > cfg.y_ratio.get())
    {
      _ctx.y_block -= _ctx.y_block % cfg.y_ratio.get();
    }

    if (cfg.x_ratio.get() > cfg.y_ratio.get())
    {
      if (auto const val = _ctx.x_block * (cfg.y_ratio.get() /
        static_cast<double>(cfg.x_ratio.get()));
        val <= _ctx.y_block)
      {
        _ctx.y_block = val;
      }
      else
      {
        _ctx.x_block = _ctx.y_block * (cfg.x_ratio.get() /
          static_cast<double>(cfg.y_ratio.get()));
      }
    }
    else if (cfg.x_ratio.get() < cfg.y_ratio.get())
    {
      if (auto const val = _ctx.y_block * (cfg.x_ratio.get() /
        static_cast<double>(cfg.y_ratio.get()));
        val <= _ctx.x_block)
      {
        _ctx.x_block = val;
      }
      else
      {
        _ctx.y_block = _ctx.x_block * (cfg.y_ratio.get() /
          static_cast<double>(cfg.x_ratio.get()));
      }
    }
    else if (_ctx.x_block != _ctx.y_block)
    {
      if (_ctx.x_block > _ctx.y_block)
      {
        _ctx.x_block = _ctx.y_block;
      }
      else
      {
        _ctx.y_block = _ctx.x_block;
      }
    }

    if (_ctx.x_block < 1)
    {
      _ctx.x_block = 1;
    }

    if (_ctx.y_block < 1)
    {
      _ctx.y_block = 1;
    }
  }
}

void Peaclock::calc_xy_begin()
{
  if (auto const val = ((_ctx.x_block * _ctx.x_blocks) + (cfg.x_space.get() *
    _ctx.x_spaces)) / 2;
    val < _ctx.width / 2)
  {
    _ctx.x_begin = (_ctx.width / 2) - val;
  }
  else
  {
    _ctx.x_begin = 0;
  }

  if (auto const val = ((_ctx.y_block * _ctx.y_blocks) + (cfg.y_space.get() * _ctx.y_spaces) +
    // (cfg.title && cfg.height_titlefmt ? cfg.height_titlefmt + 1 : 0) +
    (cfg.date && cfg.height_datefmt ? cfg.height_datefmt + cfg.date_padding.get() : 0)) / 2;
    val < _ctx.height / 2)
  {
    _ctx.y_begin = (_ctx.height / 2) - val;
  }
  else
  {
    _ctx.y_begin = 0;
  }
}

void Peaclock::fill_digital(std::size_t width, std::size_t begin, std::size_t end, Position::Type type)
{
  for (std::size_t i = 0, j = 0; i < 5; ++i)
  {
    for (std::size_t k = begin; k < end; ++j, ++k)
    {
      _ctx.buffer.at(k + (width * i)) = _symbol.at(static_cast<std::size_t>(_ctx.value.at(type))).at(j);
    }
  }
}

void Peaclock::fill_binary(std::size_t width, std::size_t col, int num)
{
  if (num >= 8)
  {
    num -= 8;
    _ctx.buffer.at(col + (0 * width)) = 1;
  }

  if (num >= 4)
  {
    num -= 4;
    _ctx.buffer.at(col + (1 * width)) = 1;
  }

  if (num >= 2)
  {
    num -= 2;
    _ctx.buffer.at(col + (2 * width)) = 1;
  }

  if (num == 1)
  {
    --num;
    _ctx.buffer.at(col + (3 * width)) = 1;
  }
}

// void Peaclock::cfg_titlefmt(std::string const& str)
// {
//   cfg.titlefmt = str;
//   cfg.height_titlefmt = OB::String::count(cfg.titlefmt, "\n") + (cfg.titlefmt.size() ? 1 : 0);
// }

void Peaclock::cfg_datefmt(std::string const& str)
{
  cfg.datefmt = str;
  cfg.height_datefmt = OB::String::count(cfg.datefmt, "\n") + (cfg.datefmt.size() ? 1 : 0);
}

bool Peaclock::cfg_timezone(std::string const& tz)
{
  cfg.timezone = tz;

  return cfg.timezone.empty() ?
    unsetenv("TZ") == 0 :
    setenv("TZ", cfg.timezone.data(), 1) == 0;
}

bool Peaclock::cfg_locale(std::string const& lc)
{
  try
  {
    auto const tmp = std::locale(lc);
    cfg.locale = lc;
  }
  catch (...)
  {
    cfg.locale.clear();

    return false;
  }

  return true;
}

void Peaclock::set_clock_value()
{
  // get the current time
  std::time_t time_raw {std::time(nullptr)};
  std::tm time_now {*std::localtime(&time_raw)};

  int hour {0};
  int min {0};
  int sec {0};

  if (timer || cfg.mode == Mode::timer)
  {
    std::tie(hour, min, sec) = timer.diff(cfg.timer_seconds);

    if (timer && hour == 0 && min == 0 && sec == 0)
    {
      timer.stop();
      cfg.timer_notify = true;
    }
  }

  if (cfg.mode == Mode::stopwatch)
  {
    std::tie(hour, min, sec) = stopwatch.hms();
  }

  else if (cfg.mode == Mode::clock)
  {
    hour = time_now.tm_hour;
    if (! cfg.hour_24)
    {
      if (hour > 12)
      {
        hour -= 12;
      }
      else if (hour == 0)
      {
        hour = 12;
      }
    }

    min = time_now.tm_min;
    sec = time_now.tm_sec;
  }

  extract_digits(hour, _ctx.value.at(Position::H), _ctx.value.at(Position::h));
  extract_digits(min, _ctx.value.at(Position::M), _ctx.value.at(Position::m));
  extract_digits(sec, _ctx.value.at(Position::S), _ctx.value.at(Position::s));

  // manually set time for asset screenshots
  // _ctx.value.at(0) = 0;
  // _ctx.value.at(1) = 1;
  // _ctx.value.at(2) = 2;
  // _ctx.value.at(3) = 3;
  // _ctx.value.at(4) = 4;
  // _ctx.value.at(5) = 5;

  // set date string
  set_date(time_now);
}

void Peaclock::set_date(std::tm const& time_now)
{
  std::ostringstream os;
  os.imbue(std::locale(cfg.locale));
  os << std::put_time(&time_now, cfg.datefmt.c_str());

  _ctx.datefmt = os.str();
}

void Peaclock::set_clock_digital()
{
  std::size_t const row_len {cfg.seconds ? 28ul : 18ul};

  fill_digital(row_len, 0, 3, Position::H);
  fill_digital(row_len, 4, 7, Position::h);
  fill_digital(row_len, 10, 13, Position::M);
  fill_digital(row_len, 10, 13, Position::M);
  fill_digital(row_len, 14, 17, Position::m);

  if (cfg.seconds)
  {
    fill_digital(row_len, 20, 23, Position::S);
    fill_digital(row_len, 24, 27, Position::s);
  }
}

void Peaclock::set_clock_binary()
{
  std::size_t const row_len {cfg.seconds ? 6ul : 4ul};

  if (cfg.hour_24)
  {
    _ctx.buffer.at(0 + (2 * (row_len + 1))) = 0;
  }
  else
  {
    _ctx.buffer.at(0 + (2 * (row_len + 1))) = -1;
  }

  for (std::size_t col {0}; col < row_len; ++col)
  {
    fill_binary(row_len + 1, col, _ctx.value.at(col));
  }
}
