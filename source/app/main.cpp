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

#include <bastapir/Bastapir.h>
#include <bastapir/bas/Keywords.h>

using namespace bastapir;
using namespace bastapir::tap;

int main(int argc, const char * argv[])
{
	auto file = FileEntry("ROMfacsilityData", FileEntry::Code, ByteArray({ 0xf3, 0xaf }));
	file.setParams({0, 32768});

	auto logger = FileErrorLogger();
	{
		auto proc_path = std::string(argv[0]);
		auto off = proc_path.rfind('/');
		if (off != proc_path.npos) {
			logger.setLogPrefix(proc_path.substr(off + 1) + ": ");
		}
	}
	
	TapArchiveBuilder builder(&logger);
	builder.addFile(file);
	auto tapContent = builder.build();
	
	printf(" Tap: ");
	for (byte b: tapContent) {
		printf("%02x ", b);
	}
	printf("\n");

	std::string nnn("4CFE");
	int val = std::stoi(nnn, nullptr, 16);
	return 0;
}
