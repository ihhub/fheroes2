/* MIT License

Copyright (c) 2018 Gregory Eslinger

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Code source: https://github.com/gregjesl/simpleson
*/

#ifndef JSON_H
#define JSON_H

/*! \file json.h
 * \brief Simpleson header file
 */

#include <exception>
#include <string>
#include <vector>
#include <cstdio>
#include <utility>
#include <stdexcept>

/*! \brief Base namespace for simpleson */
namespace json
{
	bool is_control_character(const char input);
  bool is_hex_digit(const char input);
  /*! \brief Exception used for invalid JSON keys */
	class invalid_key : public std::exception
	{
	public:
		/*! \brief The key used that was invalid */
		const std::string key;

		/*! \brief Constructor
		 *
		 * @param key The key that was not valid
		 */
		inline invalid_key(const std::string &ikey) : key(ikey) { }

		/*! \brief Destructor */
		inline ~invalid_key() throw() override { }

		/*! \brief Returns the invalid key */
		const char* what() const throw() override
		{
			return key.c_str();
		}
	};

	/*! \brief Exception used when invalid JSON is encountered */
	class parsing_error : public std::invalid_argument
	{
	public:
		/*! \brief Constructor
		 *
		 * @param message Details regarding the parsing error
		 */
		inline parsing_error(const char *message) : std::invalid_argument(message) { }

		/*! \brief Destructor */
		inline virtual ~parsing_error() throw() { }
	};

	/*\brief Alias for a list of keys */
	typedef std::vector<std::string> key_list_t;

	/* \brief Namespace for handling of JSON data types */
	namespace jtype
	{
		/*! \brief Descriptor for the type of JSON data */
		enum jtype {
			jstring, ///< String value
			jnumber, ///< Number value
			jobject, ///< JSON object
			jarray, ///< JSON array
			jbool, ///< Boolean value
			jnull, ///< Null value
			not_valid ///< Value does not conform to JSON standard
			};

		jtype peek(const char input);

		/*! \brief Geven a string, determines the type of value the string contains
		 *
		 * @param input The string to be tested
		 * @return The type of JSON value encountered
		 *
		 * \note The function will only determine the type of the first value encountered in the string.
		 */
		jtype detect(const char *input);
	}

	/*! \brief Value reader */
	class reader : protected std::string
	{
	public:
		enum push_result
		{
			ACCEPTED, ///< The character was valid. Reading should continue.
			REJECTED, ///< The character was not valid. Reading should stop.
			WHITESPACE ///< The character was whitespace. Reading should continue but the whtiespace was not stored.
		};

		/*! \brief Reader constructor */
		inline reader() : std::string(), sub_reader(NULL) { this->clear(); }

		/*! \brief Resets the reader */
		virtual void clear();

		/*! \brief Length field exposed */
		using std::string::length;

		#if __GNUC__ && __GNUC__ < 11
		inline char front() const { return this->at(0); }
		inline char back() const { return this->at(this->length() - 1); }
		#else
		/*! \brief Front method exposed */
		using std::string::front;

		/*! \brief Back method exposed */
		using std::string::back;
		#endif

		/*!\ brief Pushes a value to the back of the reader
		 *
		 * @param next the value to be pushed
		 * \returns `ACCEPTED` if the value was added to the reader, `WHITESPACE` if the input was whitespace that was not stored, and `REJECTED` is the input was invalid for the value type
		 */
		virtual push_result push(const char next);

		/*!\brief Checks the value
		 *
		 * \returns The type of value stored in the reader, or `not_valid` if no value is stored
		 */
		inline virtual jtype::jtype type() const
		{
			return this->length() > 0 ? jtype::peek(this->front()) : json::jtype::not_valid;
		}

		/*! \brief Checks if the stored value is valid
		 *
		 * \returns `true` if the stored value is valid, `false` otherwise
		 */
		virtual bool is_valid() const;

		/*! \brief Returns the stored value
		 *
		 * \returns A string containing the stored value
		 * \warning This method will return the value regardless of the state of the value, valid or not
		 */
		inline virtual std::string readout() const { return *this; }

		/*! \brief Destructor */
		inline virtual ~reader() { this->clear(); }

	protected:
		/*! \brief The subreader used during reading
		 *
		 * Arrays and objects will use a sub reader to store underlying values
		 */
		reader *sub_reader;

		/*! \brief Pushes a character to a string value */
		push_result push_string(const char next);

		/*! \brief Pushes a character to an array value */
		push_result push_array(const char next);

		/*! \brief Pushes a character to an object value */
		push_result push_object(const char next);

		/*! \brief Pushes a character to a number value */
		push_result push_number(const char next);

		/*! \brief Pushes a character to a boolean value */
		push_result push_boolean(const char next);

		/*! \brief Pushes a character to a null value */
		push_result push_null(const char next);

		/*! \brief Returns the stored state
		 *
		 * This template is intended for use with #string_reader_enum, #number_reader_enum, #array_reader_enum, and #object_reader_enum
		 */
		template<typename T>
		T get_state() const
		{
			return static_cast<T>(this->read_state);
		}

		/*! \brief Stores the reader state
		 *
		 * This template is intended for use with #string_reader_enum, #number_reader_enum, #array_reader_enum, and #object_reader_enum
		 */
		template<typename T>
		void set_state(const T state)
		{
			this->read_state = static_cast<char>(state);
		}

		/*! \brief Enumeration of the state machine for strings */
		enum string_reader_enum
		{
			STRING_EMPTY = 0, ///< No values have been read
			STRING_OPENING_QUOTE, ///< The opening quote has been read. Equivalant to #STRING_OPEN, but used for debugging the state
			STRING_OPEN, ///< The opening quote has been read and the last character was not an escape character
			STRING_ESCAPED, ///< The last character was an reverse solidus (\), indicating the next character should be a control character
			STRING_CODE_POINT_START, ///< An encoded unicode character is encountered. Expecting four following hex digits.
			STRING_CODE_POINT_1, ///< An encoded unicode character is encountered. Expecting three following hex digits (one has already been read).
			STRING_CODE_POINT_2, ///< An encoded unicode character is encountered. Expecting two following hex digits (two have already been read).
			STRING_CODE_POINT_3, ///< An encoded unicode character is encountered. Expecting one following hex digit (three has already been read).
			STRING_CLOSED ///< The closing quote has been read. Reading should cease.
		};

		enum number_reader_enum
		{
			NUMBER_EMPTY = 0, ///< No values have been read
			NUMBER_OPEN_NEGATIVE, ///< A negative value has been read as the first character
			NUMBER_ZERO, ///< A zero has been read as an integer value
			NUMBER_INTEGER_DIGITS, ///< Integer digits were the last values read
			NUMBER_DECIMAL, ///< A decimal point was the last value read
			NUMBER_FRACTION_DIGITS, ///< A decimal point and subsequent digits were the last values read
			NUMBER_EXPONENT, ///< An exponent indicator has been read
			NUMBER_EXPONENT_SIGN, ///< An exponent sign has been read
			NUMBER_EXPONENT_DIGITS ///< An exponent indicator and subsequent digits were the last values read
		};

		enum array_reader_enum
		{
			ARRAY_EMPTY = 0, ///< No values have been read
			ARRAY_OPEN_BRACKET, ///< The array has been opened
			ARRAY_READING_VALUE, ///< An array value is being read
			ARRAY_AWAITING_NEXT_LINE, ///< An array value has been read and a comma was encountered. Expecting new line.
			ARRAY_CLOSED ///< The array has been fully read. Reading should stop.
		};

		enum object_reader_enum
		{
			OBJECT_EMPTY = 0, ///< No values have been read
			OBJECT_OPEN_BRACE, ///< The object has been opened
			OBJECT_READING_ENTRY, ///< An object key value pair is being read
			OBJECT_AWAITING_NEXT_LINE, ///< An object key value pair has been read and a comma was encountered. Expecting new line.
			OBJECT_CLOSED ///< The object has been fully read. Reading should stop.
		};
	private:
		/*! \brief Storage for the current state of the reader */
		char read_state;
	};

	/*! \brief Class for reading object key value pairs */
	class kvp_reader : public reader
	{
	public:
		/*! \brief Constructor */
		inline kvp_reader() : reader()
		{
			this->clear();
		}

		/*! \brief Resets the reader */
		inline void clear() override
		{
			reader::clear();
			this->_key.clear();
			this->_colon_read = false;
		}

		/*!\ brief Pushes a value to the back of the reader
		 *
		 * \see reader::push
		 */
		push_result push(const char next) override;

		/*! \brief Checks if the stored value is valid
		 *
		 * \returns `true` if the both the key and value are valid, `false` otherwise
		 */
		inline bool is_valid() const override
		{
			return reader::is_valid() && this->_key.is_valid();
		}

		/*! \brief Reads out the key value pair
		 *
		 * \returns JSON-encoded key and JSON-encoded value seperated by a colon (:)
		 */
		std::string readout() const override;

	private:
		/*! \brief Reader for reading the key */
		reader _key;

		/*! \brief Flag for tracking whether the colon has been encountered */
		bool _colon_read;
	};

	/*! \brief Namespace used for JSON parsing functions */
	namespace parsing
	{
		/*! \brief (t)rims (l)eading (w)hite (s)pace
		 *
		 * \details Given a string, returns a pointer to the first character that is not white space. Spaces, tabs, and carriage returns are all considered white space.
		 * @param start The string to examine
		 * @return A pointer within the input string that points at the first charactor that is not white space
		 * \note If the string consists of entirely white space, then the null terminator is returned
		 * \warning The behavior of this function with string that is not null-terminated is undefined
		 */
		const char* tlws(const char *start);

		/*! \brief Reads a set of digits from a string
		 *
		 * \details This function will take an input string and read the digits at the front of the string until a character other than a digit (0-9) is encountered.
		 * @param input A string that starts with a set of digits (0-9)
		 * @return A string containing the digits
		 */
		std::string read_digits(const char *input);

		/*! \brief Decodes a string in JSON format
		 *
		 * \details The quotation mark ("), reverse solidus (\), solidus (/), backspace (b), formfeed (f), linefeed (n), carriage return (r), horizontal tab (t), and Unicode character will be unescaped
		 * @param input A string, encapsulated in quotations ("), potentially containing escaped control characters
		 * @return A string with control characters un-escaped
		 * \note This function will strip leading and trailing quotations.
		 * @see encode_string
		 */
		std::string decode_string(const char * input);

		/*! \brief Encodes a string in JSON format
		 *
		 * \details The quotation mark ("), reverse solidus (\), solidus (/), backspace (b), formfeed (f), linefeed (n), carriage return (r), horizontal tab (t), and Unicode character will be escaped
		 * @param input A string potentially containing control characters
		 * @return A string that has all control characters escaped with a reverse solidus (\)
		 * \note This function will add leading and trailing quotations.
		 * @see decode_string
		 */
		std::string encode_string(const char *input);

		/*! \brief Structure for capturing the results of parsing */
		struct parse_results
		{
			/*! \brief The type of value encountered while parsing */
			jtype::jtype type;

			/*! \brief The parsed value encountered */
			std::string value;

			/*! \brief A pointer to the first character after the parsed value */
			const char *remainder;
		};

		/*! \brief Parses the first value encountered in a JSON string
		 *
		 * @param input The string to be parsed
		 * @return Details regarding the first value encountered
		 * \exception json::parsing_error Exception thrown when the input is not valid JSON
		 */
		parse_results parse(const char *input);

		/*! \brief Template for reading a numeric value
		 *
		 * @tparam T The C data type the input will be convered to
		 * @param input The string to conver to a number
		 * @param format The format to use when converting the string to a number
		 * @return The numeric value contained by the input
		 */
		template <typename T>
		T get_number(const char *input, const char* format)
		{
			T result;
			std::sscanf(input, format, &result);
			return result;
		}

		/*! \brief Converts a number to a string
		 *
		 * @tparam The C data type of the number to be converted
		 * @param number A reference to the number to be converted
		 * @param format The format to be used when converting the number
		 * @return A string representation of the input number
		 */
		template <typename T>
		std::string get_number_string(const T &number, const char *format)
		{
			std::vector<char> cstr(6);
			int remainder = std::snprintf(&cstr[0], cstr.size(), format, number);
			if(remainder < 0) {
				return std::string();
			} else if(remainder >= static_cast<int>(cstr.size())) {
				cstr.resize(remainder + 1);
				std::snprintf(&cstr[0], cstr.size(), format, number);
			}
			std::string result(&cstr[0]);
			return result;
		}

		/*! \brief Parses a JSON array
		 *
		 * \details Converts a serialized JSON array into a vector of the values in the array
		 * @param input The serialized JSON array
		 * @return A vector containing each element of the array with each element being serialized JSON
		 */
		std::vector<std::string> parse_array(const char *input);
	}

	/*! \brief (k)ey (v)alue (p)air */
	typedef std::pair<std::string, std::string> kvp;

	/*! \class jobject
	 * \brief The class used for manipulating JSON objects and arrays
	 *
	 * \example jobject.cpp
	 * This is a basic of example of using simpleson for manipulating JSON
	 *
	 * \example rootarray.cpp
	 * This is an example of how to handle JSON where the root object is an array
	 *
	 * \example objectarray.cpp
	 * This is an example of how to handle an array of JSON objects
	 */
	class jobject
	{
	private:
		/*! \brief The container used to store the object's data */
		std::vector<kvp> data;

		/*! \brief Flag for marking whether the object is actually a JSON array
		 *
		 * \details When true, the class should be interpreted as a JSON array
		 */
		bool array_flag;

	public:
		/*! \brief Default constructor
		 *
		 * @param array If true, the instance is initialized as an array. If false, the instance is initalized as an object.
		 */
		inline jobject(bool array = false)
			: array_flag(array)
			{ }

		/*! \brief Copy constructor */
		inline jobject(const jobject &other)
			: data(other.data),
			array_flag(other.array_flag)
		{ }

		/*! \brief Destructor */
		inline virtual ~jobject() { }

		/*! \brief Flag for differentiating objects and arrays
		 *
		 * @return True if the instance represents an array, false if the instance represents an object
		 */
		bool is_array() const { return this->array_flag; }

		/*! \brief Returns the number of entries in the JSON object or array */
		inline size_t size() const { return this->data.size(); }

		/*! \brief Clears the JSON object or array */
		inline void clear() { this->data.resize(0); }

		/*! \brief Comparison operator
		 *
		 * \todo Currently, the comparison just seralizes both objects and compares the strings, which is probably not as efficent as it could be
		 */
		bool operator== (const json::jobject& other) const { return (static_cast<std::string>(*this)) == static_cast<std::string>(other); }

		/*! \brief Comparison operator */
		bool operator!= (const json::jobject& other) const { return (static_cast<std::string>(*this)) != static_cast<std::string>(other); }

		/*! \brief Assignment operator */
		inline jobject& operator=(const jobject& rhs)
		{
			this->array_flag = rhs.array_flag;
			this->data = rhs.data;
			return *this;
		}

		/*! \brief Appends a key-value pair to a JSON object
		 *
		 * \exception json::parsing_error Thrown if the key-value is incompatable with the existing object (object/array mismatch)
		 */
		jobject& operator+=(const kvp& other)
		{
			if (!this->array_flag && this->has_key(other.first)) throw json::parsing_error("Key conflict");
			if(this->array_flag && other.first != "") throw json::parsing_error("Array cannot have key");
			if(!this->array_flag && other.first == "") throw json::parsing_error("Missing key");
			this->data.push_back(other);
			return *this;
		}

		/*! \brief Appends one JSON object to another */
		jobject& operator+=(const jobject& other)
		{
			if(this->array_flag != other.array_flag) throw json::parsing_error("Array/object mismatch");
			json::jobject copy(other);
			for (size_t i = 0; i < copy.size(); i++) {
				this->operator+=(copy.data.at(i));
			}
			return *this;
		}

		/*! \brief Merges two JSON objects */
		jobject operator+(jobject& other)
		{
			jobject result = *this;
			result += other;
			return result;
		}

		/*! \brief Parses a serialized JSON string
		 *
		 * @param input Serialized JSON string
		 * @return JSON object or array
		 * \exception json::parsing_error Thrown when the input string is not valid JSON
		 */
		static jobject parse(const char *input);

		/*! \brief Parses a serialized JSON string
		 *
		 * @see json::jobject::parse(const char*)
		 */
		static inline jobject parse(const std::string& input) { return parse(input.c_str()); }

		/*! /brief Attempts to parse the input string
		 *
		 * @param input A serialized JSON object or array
		 * @param[out] output Should the parsing attempt be successful, the resultant JSON object or array
		 * @return True of the parsing attempt was successful and false if the parsing attempt was not successful
		 */
		inline bool static tryparse(const char *input, jobject &output)
		{
			try
			{
				output = parse(input);
			}
			catch(...)
			{
				return false;
			}
			return true;
		}

		/*! \brief Determines if an object contains a key
		 *
		 * @param key The key to check for
		 * @return True if the object contains the provided key and false if the object does not contain the key
		 * \note If the object represents a JSON array, then this function will always return false
		 */
		inline bool has_key(const std::string &key) const
		{
			if(this->array_flag) return false;
			for (size_t i = 0; i < this->size(); i++) if (this->data.at(i).first == key) return true;
			return false;
		}

		/*! \brief Returns a list of the object's keys
		 *
		 * @return A list of keys contained in the object. If the object is actionally an array, an empty list will be returned
		 */
		key_list_t list_keys() const;

		/*! \brief Sets the value assocaited with the key
		 *
		 * \details If the key exists, then the value is updated. If the key does not exist, then the key value pair is added to the object.
		 * @param key The key for the entry
		 * @param value The value for the entry
		 * \exception json::invalid_key Exception thrown if the object actually represents a JSON array
		 */
		void set(const std::string &key, const std::string &value);

		/*! \brief Returns the serialized value at a given index
		 *
		 * @param index The index of the desired element
		 * @return A serialized representation of the value at the given index
		 */
		inline std::string get(const size_t index) const
		{
			return this->data.at(index).second;
		}

		/*! \brief Returns the serialized value associated with a key
		 *
		 * @param key The key for the desired element
		 * @return A serialized representation of the value associated with the key
		 * \exception json::invalid_key Exception thrown if the key does not exist in the object or the object actually represents a JSON array
		 */
		inline std::string get(const std::string &key) const
		{
			if(this->array_flag) throw json::invalid_key(key);
			for (size_t i = 0; i < this->size(); i++) if (this->data.at(i).first == key) return this->get(i);
			throw json::invalid_key(key);
		}

		/*! \brief Removes the entry associated with the key
		 *
		 * @param key The key of the key value pair to be removed
		 * \note If the key is not found in the object, no action is taken
		 */
		void remove(const std::string &key);

		/*! \brief Removes the entry at the specified index
		 *
		 * @param index The index of the element to be removed
		 */
		void remove(const size_t index)
		{
			this->data.erase(this->data.begin() + index);
		}

		/*! \brief Representation of a value in the object */
		class entry
		{
		protected:
			/*! \brief A method for reference the entry's value
			 *
			 * @return A string represnting the entry's serialized value
			 */
			virtual const std::string& ref() const = 0;

			/*! \brief Converts an serialzed value to a numeric value
			 *
			 * @tparam The C data type used to represent the value
			 * @param format The format used to convert the serialized value to a numeric value
			 * @return The value as a number
			 */
			template<typename T>
			inline T get_number(const char* format) const
			{
				return json::parsing::get_number<T>(this->ref().c_str(), format);
			}

			/*! \brief Converts a serialized array of numbers to a vector of numbers
			 *
			 * @tparam The C data type used to represent the values in the array
			 * @param format The format used to convert the serialized values in the array to numeric values
			 * @return The value as a vector of numbers
			 */
			template<typename T>
			inline std::vector<T> get_number_array(const char* format) const
			{
				std::vector<std::string> numbers = json::parsing::parse_array(this->ref().c_str());
				std::vector<T> result;
				for (size_t i = 0; i < numbers.size(); i++)
				{
					result.push_back(json::parsing::get_number<T>(numbers[i].c_str(), format));
				}
				return result;
			}

		public:
			/*! \brief Returns a string representation of the value */
			inline std::string as_string() const
			{
				return json::jtype::peek(*this->ref().c_str()) == json::jtype::jstring ?
					json::parsing::decode_string(this->ref().c_str()) :
					this->ref();
			}

			/*! @see json::jobject::entry::as_string() */
			inline operator std::string() const
			{
				return this->as_string();
			}

			/*! \brief Comparison operator */
			bool operator== (const std::string& other) const { return (static_cast<std::string>(*this) == other); }

			/*! \brief Comparison operator */
			bool operator!= (const std::string& other) const { return !(static_cast<std::string>(*this) == other); }

			/*! \brief Casts the value as an integer */
			operator int() const;

			/*! \brief Casts the value as an unsigned integer */
			operator unsigned int() const;

			/*! \brief Casts teh value as a long integer */
			operator long() const;

			/*! \brief Casts the value as an unsigned long integer */
			operator unsigned long() const;

			/*! \brief Casts teh value as a char */
			operator char() const;

			/*! \brief Casts the value as a floating point numer */
			operator float() const;

			/*! \brief Casts the value as a double-precision floating point number */
			operator double() const;

			/*! \brief Casts the value as a JSON object
			 *
			 * \note This method also works for JSON arrays
			 */
			inline json::jobject as_object() const
			{
				return json::jobject::parse(this->ref().c_str());
			}

			/*! \see json::jobject::entry::as_object() */
			inline operator json::jobject() const
			{
				return this->as_object();
			}
      template<typename T> T as() const { return static_cast<T>(*this); }
			/*! \brief Casts an array of integers */
			operator std::vector<int>() const;

			/*! \brief Casts an array of unsigned integers */
			operator std::vector<unsigned int>() const;

			/*! \brief Casts an array of long integers */
			operator std::vector<long>() const;

			/*! \brief Casts an array of unsigned long integers */
			operator std::vector<unsigned long>() const;

			/*! \brief Casts an array of chars */
			operator std::vector<char>() const;

			/*! \brief Casts an array of floating-point numbers */
			operator std::vector<float>() const;

			/*! \brief Casts an array of double floating-point numbers */
			operator std::vector<double>() const;

			/*! \brief Casts an array of JSON objects */
			operator std::vector<json::jobject>() const
			{
				return this->as_array();
			}

			/*! \brief Casts an array of strings */
			operator std::vector<std::string>() const { return json::parsing::parse_array(this->ref().c_str()); }

			/*! \brief Casts an array */
			inline std::vector<json::jobject> as_array() const
			{
				const std::vector<std::string> objs = json::parsing::parse_array(this->ref().c_str());
				std::vector<json::jobject> results;
				for (size_t i = 0; i < objs.size(); i++) {
					results.push_back(json::jobject::parse(objs[i].c_str()));
				}
				return results;
			}

			/*! \brief Returns true if the value is a string */
			inline bool is_string() const
			{
				return json::parsing::parse(this->ref().c_str()).type == json::jtype::jstring;
			}

			/*! \brief Returns true if the value is a number */
			inline bool is_number() const
			{
				return json::parsing::parse(this->ref().c_str()).type == json::jtype::jnumber;
			}

			/*! \brief Returns true if the value is an object */
			inline bool is_object() const
			{
				const jtype::jtype type = json::parsing::parse(this->ref().c_str()).type;
				return type == json::jtype::jobject || type == json::jtype::jarray;
			}

			/*! \brief Returns true if the value is an array */
			inline bool is_array() const
			{
				return json::parsing::parse(this->ref().c_str()).type == json::jtype::jarray;
			}

			/*! \brief Returns true if the value is a bool */
			inline bool is_bool() const
			{
				return json::parsing::parse(this->ref().c_str()).type == json::jtype::jbool;
			}

			/*! \brief Returns true if the value is a boolean and set to true */
			inline bool is_true() const
			{
				json::parsing::parse_results result = json::parsing::parse(this->ref().c_str());
				return (result.type == json::jtype::jbool && result.value == "true");
			}

			/*! \brief Returns true if the value is a null value */
			inline bool is_null() const
			{
				return json::parsing::parse(this->ref().c_str()).type == json::jtype::jnull;
			}
		};

		/*! \brief Represents an entry as a constant value */
		class const_value : public entry
		{
		private:
			/*! \brief A copy of the entry data */
			std::string data;

		protected:
			/*! \brief Reference to the entry data
			 *
			 * @return A reference to the copied entry data
			 */
			inline const std::string& ref() const override
			{
				return this->data;
			}

		public:
			/*! \brief Constructs a proxy with the provided value
			 *
			 * @param value The entry value to copy
			 */
			inline const_value(const std::string& value)
			: data(value)
			{ }

			/*! \brief Returns another constant value from this object
			 *
			 * This method assumed the entry contains a JSON object and returns another constant value from within
			 *
			 * @param key The key of the subvalue to return
			 * @return A proxy for the value for the key
			 */
			inline const_value get(const std::string &key) const
			{
				return const_value(json::jobject::parse(this->data).get(key));
			}

			/*! \brief Returns another constant value from this array
			 *
			 * This method assumed the entry contains a JSON array and returns another constant value from within
			 *
			 * @param index The index of the subvalue to return
			 * @return A proxy for the value for the index
			 */
			inline const_value array(const size_t index) const
			{
				return const_value(json::jobject::parse(this->data).get(index));
			}
		};

		/*! \brief Represents an entry as a constant proxy to the value
		 *
		 * This proxy is more memory-efficent than a json::jobject::const_value but cannot use a JSON array as a source
		 */
		class const_proxy : public entry
		{
		private:
			/*! \brief The source object the value is referencing */
			const jobject &source;

		protected:
			/*! \brief The key for the referenced value */
			const std::string key;

			/*! \brief Returns a reference to the value */
			inline const std::string& ref() const override
			{
				for (size_t i = 0; i < this->source.size(); i++) if (this->source.data.at(i).first == key) return this->source.data.at(i).second;
				throw json::invalid_key(key);
			}

		public:
			/*! \brief Constructor
			 *
			 * @param source The JSON object the value is being sourced from
			 * @param key The key for the value being referenced
			 */
			const_proxy(const jobject &isource, const std::string& ikey) : source(isource), key(ikey)
			{
				if(isource.array_flag) throw std::logic_error("Source cannot be an array");
			}

			/*! \brief Returns another constant value from this array
			 *
			 * This method assumed the entry contains a JSON array and returns another constant value from within
			 *
			 * @param index The index of the subvalue to return
			 * @return A proxy for the value for the index
			 */
			const_value array(size_t index) const
			{
				const char *value = this->ref().c_str();
				if(json::jtype::peek(*value) != json::jtype::jarray)
					throw std::invalid_argument("Input is not an array");
				const std::vector<std::string> values = json::parsing::parse_array(value);
				return const_value(values[index]);
			}
		};

		/*! \brief A proxy that allows modification of the value
		 *
		 * \todo Currently, proxies only support JSON object and not arrays
		 */
		class proxy : public json::jobject::const_proxy
		{
		private:
			/*! \brief The parent object to be manipulated */
			jobject &sink;

		protected:
			/*! \brief Sets a number value in the parent object
			 *
			 * @tparam T The data type to be translated into JSON
			 * @param value The value to be translated to JSON
			 * @param format The format to use when translating the number
			 */
			template<typename T>
			inline void set_number(const T value, const char* format)
			{
				this->sink.set(key, json::parsing::get_number_string(value, format));
			}

			/*! \brief Stores an array of values
			 *
			 * @param values The values to store as an array
			 * @param wrap When true, the values are wrapped in quotes. When false, the values are stored as-is
			 */
			void set_array(const std::vector<std::string> &values, const bool wrap = false);

			/*! \brief Stores an array of numbers
			 *
			 * @tparam T The data type to be translated into JSON
			 * @param values The array of values to be translated into JSON
			 * @param format The format to use when translating the numbers
			 */
			template<typename T>
			inline void set_number_array(const std::vector<T> &values, const char* format)
			{
				std::vector<std::string> numbers;
				for (size_t i = 0; i < values.size(); i++)
				{
					numbers.push_back(json::parsing::get_number_string(values[i], format));
				}
				this->set_array(numbers);
			}
		public:
			/*! \brief Constructor
			 *
			 * @param source The JSON object that will be updated when a value is assigned
			 * @param key The key for the value to be updated
			 */
			proxy(jobject &isource, const std::string& ikey)
				: json::jobject::const_proxy(isource, ikey),
				sink(isource)
			{ }

			/*! \brief Assigns a string value */
			inline void operator= (const std::string& value)
			{
				this->sink.set(this->key, json::parsing::encode_string(value.c_str()));
			}

			/*! \brief Assigns a string value */
			inline void operator= (const char* value)
			{
				this->operator=(std::string(value));
			}

			/*! \brief Assigns an integer */
			void operator=(const int input) { this->set_number(input, "%i"); }

			/*! \brief Assigns an unsigned integer */
			void operator=(const unsigned int input) { this->set_number(input, "%u"); }

			/*! \brief Assigns a long integer */
			void operator=(const long input) { this->set_number(input, "%li"); }

			/*! \brief Assigns a long unsigned integer */
			void operator=(const unsigned long input) { this->set_number(input, "%lu"); }

			/*! \brief Assigns an character */
			void operator=(const char input) { this->set_number(input, "%c"); }

			/*! \brief Assigns an double floating-point integer  */
			void operator=(const double input) { this->set_number(input, "%e"); }

			/*! \brief Assigns an floating-point integer  */
			void operator=(const float input) { this->set_number(static_cast<double>(input), "%e"); }

			/*! \brief Assigns a JSON object or array */
			void operator=(json::jobject& input)
			{
				this->sink.set(key, static_cast<std::string>(input));
			}

			/*! \brief Assigns an array of integers */
			void operator=(const std::vector<int>& input) { this->set_number_array(input, "%i"); }

			/*! \brief Assigns an array of unsigned integers */
			void operator=(const std::vector<unsigned int>& input) { this->set_number_array(input, "%u"); }

			/*! \brief Assigns an array of long integers */
			void operator=(const std::vector<long>& input) { this->set_number_array(input, "%li"); }

			/*! \brief Assigns an array of unsigned long integers */
			void operator=(const std::vector<unsigned long>& input) { this->set_number_array(input, "%lu"); }

			/*! \brief Assigns an array of characters */
			void operator=(const std::vector<char>& input) { this->set_number_array(input, "%c"); }

			///*! \brief Assigns an array of floating-point numbers */
			//void operator=(const std::vector<float> input) { this->set_number_array(input, "%e"); }

			/*! \brief Assigns an array of double floating-point numbers */
			void operator=(const std::vector<double>& input) { this->set_number_array(input, "%e"); }

			/*! \brief Assigns an array of strings */
			void operator=(const std::vector<std::string>& input) { this->set_array(input, true); }

			/*! \brief Assigns an array of JSON objects */
			void operator=(const std::vector<json::jobject>& input)
			{
				std::vector<std::string> objs;
				for (size_t i = 0; i < input.size(); i++)
				{
					objs.push_back(static_cast<std::string>(input[i]));
				}
				this->set_array(objs, false);
			}

			/*! \brief Sets a boolean value
			 *
			 * This method is required because operator=(bool value) conflict with number-based assignments
			 */
			inline void set_boolean(const bool value)
			{
				if (value) this->sink.set(key, "true");
				else this->sink.set(key, "false");
			}

			/*! Sets a null value */
			inline void set_null()
			{
				this->sink.set(key, "null");
			}

			/*! Clears the value */
			inline void clear()
			{
				this->sink.remove(key);
			}
		};

		/*! \brief Returns an element of the JSON object
		 *
		 * @param key The key of the element to be returned
		 * @return A proxy for the value paired with the key
		 * \exception json::invalid_key Exception thrown if the object is actually a JSON array
		 */
		inline virtual jobject::proxy operator[](const std::string& ikey)
		{
			if(this->array_flag) throw json::invalid_key(ikey);
			return jobject::proxy(*this, ikey);
		}

		/*! \brief Returns an element of the JSON object
		 *
		 * @param key The key of the element to be returned
		 * @return A proxy for the value paired with the key
		 * \exception json::invalid_key Exception thrown if the object is actually a JSON array
		 */
		inline virtual const jobject::const_proxy operator[](const std::string& key) const
		{
			if(this->array_flag) throw json::invalid_key(key);
			return jobject::const_proxy(*this, key);
		}

		/*! \brief Returns the value of an element in an array
		 *
		 * @param index The index of the element to be returned
		 * @return A proxy for the value
		 *
		 * \note While this method is intended for JSON arrays, this method is also valid for JSON objects
		 */
		inline const jobject::const_value array(const size_t index) const
		{
			return jobject::const_value(this->data.at(index).second);
		}

		/*! \see json::jobject::as_string() */
		operator std::string() const;

		/*! \brief Serialzes the object or array
		 * \note The serialized object or array will be in the most compact form and will not contain any extra white space, even if the serialized string used to generate the object or array contained extra white space.
		 */
		inline std::string as_string() const
		{
			return this->operator std::string();
		}

		/*! \brief Returns a pretty (multi-line indented) serialzed representation of the object or array
		 *
		 * @param indent_level The number of indents (tabs) to start with
		 * @return A "pretty" version of the serizlied object or array
		 */
		std::string pretty(unsigned int indent_level = 0) const;
	};
}

#endif // !JSON_H
