all: bash-pinyin-completion

bash-pinyin-completion: bash-pinyin-completion.cpp
	c++ -std=c++23 bash-pinyin-completion.cpp -o bash-pinyin-completion

clean:
	rm -f bash-pinyin-completion
