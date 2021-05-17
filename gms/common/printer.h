#ifndef GMS_PRINTER_H
#define GMS_PRINTER_H

#include <iostream>
#include <vector>
#include <string>
#include <sstream>

namespace GMS {

/**
 * @brief Simple printer that collects output and
 * prints in to a single line, formatted with a prefix
 * and separators.
 * 
 * Authors: Yannick Schaffner
 */
class Printer
{
private:
    std::stringstream _ss;
    bool _putSeparator;

public:

    /**
     * @brief Separator to use. Default is a whitespace (" ")
     *
     */
    std::string Separator;

    /**
     * @brief Prefix to use. Default is "@@@".
     *
     */
    std::string Prefix;

    Printer() : _putSeparator(false), Separator(" "), Prefix("@@@")
    {}

    template<typename T>
    void Enqueue(const T &t)
    {
        if(_putSeparator)
        {
            _ss << Separator;
        }
        else
        {
            _ss << Prefix << " ";
            _putSeparator = true;
        }
        _ss << t;
    }

    /**
     * @brief Adds t to the printer queue.
     *
     * @tparam T
     * @param t Must be printable to a stringstream.
     */
    template<typename T>
    void operator<<(const T &t)
    {
        Enqueue(t);
    }

    /**
     * @brief Adds t and ts... to the printer queue
     *
     * @tparam T
     * @tparam Ts
     * @param t Must be printable to a stringstream
     * @param ts All arguments must be printable to a stringstream
     */
    template<typename T, typename...Ts>
    void Enqueue(const T &t, const Ts&...ts)
    {
        Enqueue(t);
        Enqueue(ts...);
    }

    /**
     * @brief Prints the queue to a string and empties it.
     *
     * @return std::string
     */
    std::string str()
    {
        std::string result = _ss.str();
        std::stringstream().swap(_ss);
        return result;
    }

    /**
     * @brief Prints the queue to ostream and empties it.
     *
     * @param output
     * @param printer
     * @return std::ostream&
     */
    friend std::ostream &operator<<(std::ostream &output, Printer &printer)
    {
        output << printer._ss.str();
        std::stringstream().swap(printer._ss);
        printer._putSeparator = false;
        return output;
    }

};

}

#endif