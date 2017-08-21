
#include <cstdlib>
#include <vector>
#include <list>
#include <string>
#include <iostream>
#include <set>

using namespace std;


/////////////////////////////////////////////////////////////
//// Word Manager
class WordManager{

public:

  WordManager(vector<string> &v){
    maxLength = CalculateMaxLength(v);
    // if there are some valid words
    if(maxLength){
      archive.resize(maxLength);
      for(int i = 0; i < v.size(); i++) archive[v[i].length()-1].insert(v[i]);
    }
  }

  bool Empty(){
    return !maxLength;
  }

  /* if no such words with same length returns empty vector
   * if such words exist, then returns indexes
   */
  vector<string> ExtractLongestOfSameLength(int maxLength, int number){
    vector<string> result;
    if(!Empty()){
      if(maxLength >= this->maxLength) maxLength = this->maxLength;

      while((maxLength > 0) && (archive[maxLength-1].size() < number)) maxLength--;

      if(maxLength){
        result.resize(number);
        for(int i = 0; i < number; i++){
          result[i] = *(archive[maxLength-1].begin());
          archive[maxLength-1].erase(archive[maxLength-1].begin());
        }

        if(maxLength == this->maxLength){
          while(this->maxLength > 0 && archive[this->maxLength-1].empty()) this->maxLength--;
        }
      }
    }
    return result;
  }

  /* if no such word returns -1
   * if such word exists, then returns index
   */
  string ExtractLongest(int maxLength){
    string result = "";
    if(!Empty()){
      if(maxLength >= this->maxLength) maxLength = this->maxLength;

      while(maxLength > 0 && archive[maxLength-1].empty()) maxLength--;

      if(maxLength){
        result = *(archive[this->maxLength-1].begin());
        archive[this->maxLength-1].erase(archive[this->maxLength-1].begin());

        if(maxLength == this->maxLength){
          while(this->maxLength > 0 && archive[this->maxLength-1].empty()) this->maxLength--;
        }
      }
    }
    return result;
  }

  string ExtractLongest(){
    return ExtractLongest(maxLength);
  }

private:
  // sorting indexes of words by length of words (between 0 and maxWordLength-1)
  vector<set<string> > archive;
  int maxLength;

  static int CalculateMaxLength(vector<string> &v){
    int result = 0;
    for(int i = 0; i < v.size(); i++)
      if(v[i].length() > result) result = v[i].length();
    return result;
  }

};


/////////////////////////////////////////////////////////////
// main class
class BeautifulCrossword {

public:

    vector<string> generateCrossword(int n, vector<string> word, vector<int> weight);

};

vector<string> BeautifulCrossword::generateCrossword(int n, vector<string> word, vector<int> weight){
    vector<string> result(n);
    for(int i = 0; i < n; i++) result[i].resize(n,'.');

    WordManager wm(word);
    vector<string> buf, strBuf(8);
    vector<list<string> > mainBuf(8);

    for(int k = (n-1)/2-1; k >= 0 ; k--){

      // forming list buffer
      int p = k;
      while(1){
        buf = wm.ExtractLongestOfSameLength(p,8);
        if(!buf.empty()){
          p-= buf[0].length() + 1;
          mainBuf[0].push_back(buf[0]);
          mainBuf[1].push_back(buf[1]);
          mainBuf[2].push_back(buf[2]);
          mainBuf[3].push_front(buf[3]);
          mainBuf[4].push_back(buf[4]);
          mainBuf[5].push_front(buf[5]);
          mainBuf[6].push_front(buf[6]);
          mainBuf[7].push_front(buf[7]);
        }
        else break;

        if(p < 0) break;
      }

      if(!mainBuf[0].empty()){
        for(int i = 0; i < 8; i++) strBuf[i] = "";
        while(1){
          for(int i = 0; i < 8; i++){
            strBuf[i] += mainBuf[i].back();
            mainBuf[i].pop_back();
          }
          if(!mainBuf[0].empty()){
            for(int i = 0; i < 8; i++) strBuf[i] += '.';
          }else break;
        }

        int length = strBuf[0].length();

        for(int h = 0; h < length; h++){
          result[k][k-length+h] = strBuf[0][h];
          result[k-length+h][k] = strBuf[1][h];
          result[k-length+h][n-k-1] = strBuf[2][h];
          result[k][n-k+h] = strBuf[3][h];
          result[n-k-1][k-length+h] = strBuf[4][h];
          result[n-k+h][k] = strBuf[5][h];
          result[n-k+h][n-k-1] = strBuf[6][h];
          result[n-k-1][n-k+h] = strBuf[7][h];
        }
        k--;  // indent
      }
    }
    return result;
}




