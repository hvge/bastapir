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

#include <bastap/Bastap.h>

using namespace bastap;
using namespace bastap::tap;

int main(int argc, const char * argv[])
{
	auto file = FileEntry("ROM", FileEntry::Code, ByteArray({ 0xf3, 0xaf }));
	file.setParams({0, 32768});

	TapGenerator generator;
	generator.addFile(file);
	auto tapContent = generator.generate();
	
	printf(" Tap: ");
	for (byte b: tapContent) {
		printf("%02x ", b);
	}
	printf("\n");
	return 0;
}