#include <view/ConnectSuggestionsContextCommand.h>
#include <view/ConnectSuggestionsContext.h>

namespace Helix {
	namespace View {
		ConnectSuggestionsContextCommand::ConnectSuggestionsContextCommand() {

		}

		ConnectSuggestionsContextCommand::~ConnectSuggestionsContextCommand() {

		}

		MPxContext *ConnectSuggestionsContextCommand::makeObj() {
			return new ConnectSuggestionsContext();
		}

		void *ConnectSuggestionsContextCommand::creator() {
			return new ConnectSuggestionsContextCommand();
		}
	}
}
