//
// Copyright 2018 Juraj Durech <durech.juraj@gmail.com>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <bastapir/common/Tokenizer.h>
#include <assert.h>
#include <ctype.h>

namespace bastapir
{

#define _CHECK_LE() assert(_state.updateLineEnd == false);

	// MARK: - Constructor
	
	Tokenizer::Tokenizer(ErrorLogging * logger) :
		_stop_at_lf(false),
		_log(logger)
	{
	}
	
	// MARK: - Config
	
	void Tokenizer::setErrorLogging(ErrorLogging * logger)
	{
		_log = logger;
	}
	
	void Tokenizer::resetTo(const Range range)
	{
		_str = range;
		reset();
	}
	
	void Tokenizer::resetTo(const iterator begin, const iterator end)
	{
		resetTo(Range { begin, end});
	}
	
	void Tokenizer::reset()
	{
		_state.pos 				= _str.begin;
		_state.line.begin 		= _str.begin;
		_state.line.end 		= _str.begin;
		_state.updateLineEnd	= true;
		_state.lineNumber		= 0;
		
		resetCapture();
		
		if (_stop_at_lf) {
			updateLineEnd();
		}
	}
	
	void Tokenizer::resetLine()
	{
		_state.pos = _state.line.begin;
		resetCapture();
	}
	
	void Tokenizer::setStopAtLineEnd(bool stop)
	{
		_stop_at_lf = stop;
	}
	
	bool Tokenizer::isStopAtLineEnd() const
	{
		return _stop_at_lf;
	}
	
	Tokenizer::State Tokenizer::state() const
	{
		return _state;
	}
	
	void Tokenizer::restoreState(const State & state)
	{
		_state = state;
	}
	
	Tokenizer::PositionInfo Tokenizer::positionInfo() const
	{
		PositionInfo pi;
		pi.lineNumber   = _state.lineNumber;
		pi.offsetAtLine = std::distance(_state.line.begin, _state.pos);
		return pi;
	}
	
	Tokenizer::PositionInfo Tokenizer::positionInfoForLog() const
	{
		PositionInfo pi;
		pi.lineNumber   = 1 + _state.lineNumber;
		pi.offsetAtLine = 1 + std::distance(_state.line.begin, _state.pos);
		return pi;
	}
	
	
	// MARK: - Walking over string
	
	bool Tokenizer::isEnd() const
	{
		if (!_stop_at_lf) {
			return isRealEnd();
		}
		_CHECK_LE();
		return _state.pos == _state.line.end;
	}
	
	Tokenizer::iterator Tokenizer::position() const
	{
		return _state.pos;
	}
	
	const Tokenizer::Range & Tokenizer::limit() const
	{
		if (_stop_at_lf) {
			_CHECK_LE()
			return _state.line;
		}
		return _str;
	}
	
	char Tokenizer::getChar()
	{
		if (!isEnd()) {
			char c = *(_state.pos++);
			if (c == 0) {
				if (_log) {
					_log->error("Tokenizer: NUL character detected in string.");
				}
			}
			return c;
		}
		return 0;
	}
	
	char Tokenizer::charAt(difference offset) const
	{
		if (offset >= 0) {
			auto dist = std::distance(_state.pos, limit().end);
			if (offset >= dist) {
				return 0;
			}
		} else if (offset < 0) {
			auto dist = std::distance(_state.pos, limit().begin);
			if (offset < dist) {
				return 0;
			}
		}
		char c = *(_state.pos + offset);
		if (c == 0 && _log) {
			_log->error("Tokenizer: NUL character detected in string.");
		}
		return c;
	}
	
	char Tokenizer::isCharAt(char c, difference offset) const
	{
		return charAt(offset) == c;
	}
	
	// MARK: - Move forwad / backward
	
	bool Tokenizer::movePosition(difference offset)
	{
		if (offset >= 0) {
			auto dist = std::distance(_state.pos, limit().end);
			if (offset > dist) {
				return false;
			}
		} else if (offset < 0) {
			auto dist = std::distance(_state.pos, limit().begin);
			if (offset < dist) {
				return false;
			}
		}
		_state.pos += offset;
		return true;
	}
	
	bool Tokenizer::nextLine()
	{
		if (!_stop_at_lf) {
			updateLineEnd();
		} else {
			_CHECK_LE();
		}
		// Set position directly to the end of line
		_state.pos = _state.line.end;
		char c = realGetChar();
		if (c != 0) {
			if (c == '\n') {
				// UNIX, do nothing...
			} else if (c == '\r') {
				// Handle crappy MS-DOS & Descendants
				c = realGetChar();
				if (c != '\n') {
					if (_log) {
						_log->error("Tokenizer: Invalid CR-LF sequence detected.");
					}
					_state.pos--;
				}
			}
			_state.lineNumber++;
		}
		// Reset line state
		_state.line.begin = _state.pos;
		_state.line.end   = _state.pos;
		_state.updateLineEnd = true;
		// 
		if (_stop_at_lf) {
			updateLineEnd();
			resetCapture();
		}
		
		return !isRealEnd();
	}
	
	Tokenizer::Range Tokenizer::line()
	{
		updateLineEnd();
		return _state.line;
	}
	
	
	// MARK: - Search
	
	bool Tokenizer::skipWhitespace()
	{
		return skipWhile(isspace);
	}
	
	bool Tokenizer::skipWhile(int (*function)(int))
	{
		assert(function != nullptr);
		while (true) {
			char c = charAt();
			if (c == 0) {
				return false;
			}
			if (!function(c)) {
				return true;
			}
			movePosition();
		}
	}
	
	bool Tokenizer::searchFor(int (*function)(int))
	{
		assert(function != nullptr);
		while (true) {
			char c = getChar();
			if (c == 0) {
				return false;
			}
			if (function(c)) {
				return true;
			}
		}
	}

	
	// MARK: - Range capture
	
	void Tokenizer::resetCapture()
	{
		_state.capture.begin = _state.pos;
		_state.capture.end   = _state.pos;
	}
	
	Tokenizer::Range Tokenizer::capture()
	{
		_state.capture.end = _state.pos;
		return _state.capture;
	}
	
	Tokenizer::Range Tokenizer::captureAndReset()
	{
		auto c = capture();
		resetCapture();
		return c;
	}
	
	void Tokenizer::_debugInfo() const
	{
		Range dbg_lim = limit();
		std::string tmp;
		std::string pos;
		std::string rng;
		auto it = dbg_lim.begin;
		while (it != dbg_lim.end) {
			char c = *it;
			if (c == '\n') {
				tmp += u8"↵";
			} else if (isspace(c)){
				tmp += u8"·";
			} else {
				tmp += c;
			}
			pos += it == _state.pos ? '^' : ' ';
			rng += it >= _state.capture.begin && it < _state.capture.end ? '~' : ' ';
			it++;
		}
		printf("  | Stop at LF is %s\n", _stop_at_lf ? "ON " : "OFF");
		printf("  | TEXT : %s |\n", tmp.c_str());
		printf("  |      : %s | < position\n", pos.c_str());
		printf("  |      : %s | < capture\n", rng.c_str());
	}
	
	
	// MARK: - Private methods
	
	char Tokenizer::realCharAt(difference offset) const
	{
		if (offset > 0) {
			if (offset >= std::distance(_state.pos, _str.end)) {
				return 0;
			}
		} else {
			if (offset < std::distance(_state.pos, _str.begin)) {
				return 0;
			}
		}
		return *(_state.pos + offset);
	}
	
	bool Tokenizer::isRealEnd() const
	{
		return _state.pos == _str.end;
	}
	
	char Tokenizer::realGetChar()
	{
		if (!isRealEnd()) {
			return *(_state.pos++);
		}
		return 0;
	}
	
	void Tokenizer::updateLineEnd()
	{
		if (_state.updateLineEnd) {
			_state.line.end = _state.line.begin;
			while (_state.line.end != _str.end) {
				auto c = *_state.line.end;
				if (c == '\n' || c == '\r') {
					break;
				}
				++_state.line.end;
			}
			_state.updateLineEnd = false;
		}
	}
	
} // bastapir

