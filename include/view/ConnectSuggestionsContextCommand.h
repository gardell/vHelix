#ifndef _CONNECT_SUGGESTIONS_CONTEXT_COMMAND_H_
#define _CONNECT_SUGGESTIONS_CONTEXT_COMMAND_H_

#include <Definition.h>

#include <maya/MPxContextCommand.h>

#define MEL_CONNECT_SUGGESTIONS_CONTEXT_COMMAND "connectSuggestionsContext"

namespace Helix {
	namespace View {
		class ConnectSuggestionsContextCommand : public MPxContextCommand {
		public:
			ConnectSuggestionsContextCommand();
			virtual ~ConnectSuggestionsContextCommand();

			virtual MPxContext *makeObj();

			static void *creator();
		};
	}
}

#endif /* N _CONNECT_SUGGESTIONS_CONTEXT_COMMAND_H_ */