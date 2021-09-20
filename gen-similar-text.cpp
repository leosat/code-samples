// ---------------------------
// Generate semi-random text based on adjacent lexemes sequences extracted from a given source.
// ---------------------------
// Minimalistic, C++17 single STL container-based solution, no custom containers.
// Store unique lexemes once for minimal memory footprint.
// Alternative approches are listed in comments.
// ---------------------------
// Warning: No coding guidelines have been given to the author.
//          The resulting program code style may be considered harmful by some individuals.
//          Providing this code the author has no intent to cause any damage and takes no responsibility for any
//          negative side-effects caused by attempts to interpret it by any life being in existance.
// ---------------------------

#include <iostream>
#include <fstream>

#include <string>
#include <vector>
#include <unordered_map>

#include <regex>
#include <random>

const auto kSoftLexLimit = 500;
const auto kStartingLex = ".";
const auto kTerminatorLex = kStartingLex;
const auto kFileName = "text.txt";

// Yes... some macro. Can make a stream-like class instead, but don't want to add more lines of code here. NB: For multithreaded version (if requested) we'd put synchronization in console outputs.
#define DBG(t) std::cout << "[d] " << t
#define DBG_APPEND(t) std::cout << t
#define ERR(t) std::cout << "[e] " << t; abort ()
#define OUT(t) std::cout << t

class Distr
{
private:
    using DistrModel = std::unordered_map<
          std::string, // Lexeme string is being stored only once here.
          std::vector<const std::string*>
              // Naive approach: storing all of the following non-unique alternatives and using
              // std::uniform_distribution to peek.
              // ----
              // We could store the unique following alternatives sorted by "times seen count"
              // ("weight") and use std::discrete_distribution to peek.
    >;

    DistrModel m_model;
    std::unordered_map<int, std::uniform_int_distribution<>> m_uniform_sitribution_states;

    std::random_device m_rd;
    std::mt19937 m_gen {m_rd()};

public:
    size_t uniqueLexCount () const { return m_model.size (); }

    const std::string* const add(const std::string& curLex, const std::string& nextLex)
    {
        // We keep only one copy of each unique token string (as a key in an std::unordered_map).
        // Any further referentials to those tokens are valid due to
        // the standard guarantee: http://eel.is/c++draft/unord.req#9
        const auto nextLexPtr = std::addressof(m_model.try_emplace(nextLex).first->first);
        m_model.try_emplace(curLex).first->second.push_back(nextLexPtr);
        return nextLexPtr;
        // NB: I perfectly admit that this usage of std::unordered_map may surprise some and this style of code may
        // be considered generally harmful by some coding guidelines or religions, but... I haven't been given any
        // coding guidelines to execute this little task, so...
    };

    const std::string& nextLex(const std::string& curLex)
    {
        const auto& distrData = m_model[curLex];
        const auto size = distrData.size();

        if (!size) {
            ERR("Can't find next token for " << curLex << std::endl);
        }

        // Well... we keep uniform random number distribution state per number of elements.
        return *distrData[
            m_uniform_sitribution_states.try_emplace(
                size - 1, 0, size - 1
            ).first->second(m_gen)
        ]; // See the NB: above.
    };

    void dump()
    {
        DBG_APPEND(std::endl);
        DBG("---- Sequential pairs distribution model dump ----" << std::endl);
        for (const auto& token: m_model) {
            DBG(token.first << ":( ");
            for (const auto& nextAltTokenNamePtr: token.second)
                DBG_APPEND(*nextAltTokenNamePtr << " ");
            DBG_APPEND(")" << std::endl);
        }
    }
};

int main()
{
    Distr distr;

    std::ifstream in {kFileName};

    if(!in.is_open()) {
        ERR("Can't open the file specified: " << kFileName);
    };

    //-------------------------------
    // PARSE THE INPUT:
    //-------------------------------
    DBG_APPEND(std::endl);
    DBG("----Input----" << std::endl);

    const std::string* prevChunkLastLexStrPtr = nullptr;
    const std::string* lastLexStrPtr = nullptr;
    std::string chunk;

    while(in >> chunk) // Reading input per whitespace-separated "chunks".
    {
        auto prevIt = std::sregex_token_iterator {};

        DBG(chunk << " @< ");

        // Parse chunk contents based on regex into separate lexemes and process for further usage.
        static const auto lexRx = std::regex {
            "([[:w:]]+[-'][[:w:]]+)|"
            "([[:w:]]+)|"
            "([.]{3})|"
            "([[:punct:]])"};

        for (auto it = std::sregex_token_iterator {chunk.begin (), chunk.end (), lexRx};;)
        {
            DBG_APPEND(*it);

            lastLexStrPtr = distr.add(
                (prevChunkLastLexStrPtr ? *prevChunkLastLexStrPtr : 
                    prevIt == std::sregex_token_iterator {} ?
                        kStartingLex : 
                            static_cast<std::string>(*prevIt)),
                    *it);
                    
            prevIt = it;
            if(++it == std::sregex_token_iterator {}) {
                prevChunkLastLexStrPtr = lastLexStrPtr;
                break;
            } else {
                if (prevChunkLastLexStrPtr) prevChunkLastLexStrPtr = nullptr;
                DBG_APPEND(" / ");
            }
        }

        DBG_APPEND(" >@ " << std::endl);
    }

    const auto uniqueLexCount = distr.uniqueLexCount ();

    if(!uniqueLexCount)
    {
        DBG("Got no parsable data on the input.");
        return 0;
    }

    DBG("Chunk buffer capacity has grown up to " << chunk.capacity () << std::endl);

    distr.dump();

    //-------------------------------
    // GENERATE:
    //-------------------------------
    DBG_APPEND(std::endl);
    DBG("----Generated----" << std::endl);
    auto is_punctation = [] (const std::string& lex) {
        return std::regex_search(lex, std::regex{"^(([.]{3})|[[:punct:]])$"});
    };

    auto lex = distr.nextLex(kStartingLex);
    int i = 0;
    while (true) 
    {
        OUT(lex);
        
        if (!is_punctation(
            lex = distr.nextLex(lex) // Pushing the limits of coding guidelines...
        )) OUT(" ");

        if (++i > kSoftLexLimit && lex == kTerminatorLex) 
        {
            OUT(lex << std::endl);
            i++;
            break;
        }
    };

    DBG_APPEND(std::endl);
    DBG("Success. Generated text of " << i << " lexemes from " << uniqueLexCount << " unique ones." << std::endl);
    return 0;
}
