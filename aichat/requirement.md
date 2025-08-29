Create a aichatclient helper class which will have few constraints.

1. it will get a continuous stream of sentence from a asr engine . As the chat client backend dosen't have streaming capability it will wait for all the sentnce to stream and after it receives a end of conversion command it will send it to clinet backend for processing and when get the result returns back to the user.

2. The problem with this approch is the long delay as asr has to detect the end of the conversation by waiting for few seconds of silence and then sends the end of conversation to chat client which sends the whole conversation it accumulates till now to the backend which could be in cloude for processing.

3. Plan a optimum implementation to reduce the latency giving the constraints of chat backend dosen't support streaming conversation