//	SUBSYSTEM:		A "nice" class for illustrating C++ operators
//	FILE:				niceclas.h
//	AUTHOR:			Chris Ahlstrom (ahlstroc@bellsouth.net)
//	DATE:				04/28/98
// TAB STOPS:		3
//

// *****************************************************************************
// NICE CLASSES AND CODING CONVENTIONS
//
//		This module is not a useable C++ header file; instead, it is
// a fast tutorial on writing class functions and friendly global
// functions.  In addition, it illustrates coding conventions pretty
// similar to what we'd like to use in our everyday code.
//
//		It declares all the items of a "nice" class and a "regular" class.
// Demonstrates the basics of some useful techniques for improving
// the consistency and maintainability of a class.
//
//		For this discussion, assume that all the following occur as inline
// functions in the class declaration
//
//				class X : public B {};
//
// Let class T represent a built-in type; let B and Y represent any other type.
// Let instances of each class be denoted by the same letter, in lower case.
//
//		The following code fragments represent some of the more important
// constructors, operators, and destructors that can be written, and the
// things needing to be done by each of them for efficiency, safety, and
// maintainability.
//
//		Margaret Ellis and her colleagues group the functions in the categories
// of "regular" and "nice".  No, "regular" is /not/ what's normal for
// you, although many people seem to think that concepts must follow their
// biases.  These two terms summarize classes that have behavior consistent
// with that of the built-in classes.  There should be few surprises in
// the usage of a regular or nice class.
//
//		Of course, the cynical view of programmers like myself and Dogbert
// (Dilbert, of course, being an engineer, doesn't know how to program...
// he just thinks he does) is that the use of the techniques alluded to
// herein will cause a great reduction in "job security", because other
// programmers will be better able to read your code, and thus take your
// job away from you.
//
//		Besides, you know in your heart of hearts which of the following
// scenarios your boss would prefer:
//
//		1.	You take your time, and carefully construct your design.
//			You take longer than you said you would, and the code has
//			bugs in it anyway.  The client thinks your company is stupid.
//			He is very leery of dishing out more money.
//
//		2.	You rush through the project, cutting-and-pasting like
//			a kindergartener, and turn the project in on time.  It has
//			bugs in it, lots of them.  The client thinks your company
//			is the normal bunch of development screwups he's come to
//			expect.  Your boss convinces him that true completion is
//			very close, so he keeps dishing out the money.
//
//		3.	You take your time, but are snappy about it, carefully
//			construct your design, and finish on time.  The product
//			has few bugs.  The client is totally pleased.
//
// However, you know that scenario 3 is pure fantasy, so you know
// what really happens.  Besides, you know that the client will still
// give the job to the low bid next time, and you'll be out of work,
// no matter how well you did.
//
//		Let's get to the characteristics of regular and nice classes.
//
// -------------------------------------------------
// Regular classes provide the following functions:
// -------------------------------------------------
//
// copy constructor				Construct an X whose value is the same as x
// destructor						Destroy this X object
// principal assignment op.	Set this object to x & return a reference to it
// equality op						Return true if & only if x1 & x2 have same value
// inequality op					Return true if & only if x1 & x2 not the same
//
// -------------------------------------------------
// Nice classes provide the following functions:
// -------------------------------------------------
//
// default constructor			Construct an object with a “null” value
// copy constructor				Construct an X whose value is the same as x
// destructor						Destroy this X object
// assignment op.					Set this object to x and return a reference to it
// equality op						Return true if & only if x1 and x2 have same value
//
//		Note that we provide a relatively simple class declaration, followed
// by a well-documented inline definition for each function.  This should
// give you quite a jump start on your own nice classes.  Thanks to the
// books of Bjarne Stroustrop, Scott Meyers, and Stan Lippman, as well
// as code by Paul Pedriana, and an article by Ellis.
//
// -------------------------------------------------
// Semantics of copying and assignment
// -------------------------------------------------
//
//		Here, we discuss the copy constructor, the principal assignment
// operator (PAO), a copy() function, and a clone() function.  The copy() and
// clone() functions are useful in containing common code used in the
// writing of the copy constructor and principle assignment operator.
//
//												copy		principle
// Item									constructor	assignment	copy()	 clone()
//
// Destination exists					 no			yes		 yes			no
//	Need to delete old items			 no			yes		 yes			no
// Need to create new items			 yes			yes		 yes			yes
//
//		You might call this "the Tao of PAO".  In any case, GUI frameworks
// like MFC and OWL often hide the copy constructor and assignment
// operator because writing them correctly can be hard work!
//
// -------------------------------------------------
// Coding Conventions
// -------------------------------------------------
//
//		Lots of lessons packed into this module:
//
//		0.	Use the correct header style on all modules, including the
//			BPR and BPG files.
//
//		1. Use tab characters, with the tab stops of your editor
//			set to increments of 3.  Set up your editor or IDE right
//			now!
//
//			Spaces could be used instead of tabs.  Tabs can give one the
//			problem of raggedness at other increments than 3, but they
//			are easier to control and find than spaces.
//
//		2.	Use white space wisely and to good effect.  Surround operators
//			with a space when possible.
//
//		3.	Use the carriage return to increase readability.
//
//		4.	Comments.
//
//			a.	Use short side comments when possible, and line them up
//				at the same tab stop.
//
//			b.	We have two ways to comment lines of code.  Pick whichever
//				works best for the kind of code your writing.
//
//				i. /* Put the comment right above code.  Good with one-liners. */
//					category = NICE_CLASS;
//
//			  ii. //
//					// Embed the comment as a paragraph before a number of lines
//					// of code.  This style is good when you want to explain a
//					// number of lines of code using a single, long comment.
//					// Extra blank comment lines can be added if desired.
//					//
//
//					category = NICE_CLASS;
//					for (int ci = 0; ci < category; ci++)
//					{
//							much_code();	// a routine thing to do!
//					}
//
//			c.	Comment on and explain all parameters and return values.
//				Don't allow the reader to make incorrect guesses as to
//				what the parameters means.
//
//		5.	Put the return type of functions on a line by itself.  This
//			habit makes it easier to find declarations with a real
//			programmer's editor (one that can search for the beginning of 
//			a line), and also makes long declarations easier to read.
//
//		6.	Naming conventions.  This area seems to garner the most
//			controversy, so the following items are just suggestions.
//
//			a. Try to use lower case for local variables.
//
//			b.	Avoid use of warts unless it is deeply ingrained in your
//				psyche.  Some common warts:
//
//				o	sz		string terminated by an ASCII zero
//				o	lpsz	long pointer to string terminated by an ASCII zero
//				o	b		boolean variable
//				o	i		integer variable -- holy FORTRAN!
//				o	m_		a member variable of a Microsoft class
//				o	_ 		Appended or prepended to certain members
//
//			c.	Use mixed case for members of a class, or whereever it
//				will distinguish variables with good effect.
//
//		7.	Adopt a consistent and clean function indentation style.  Here
//			are two.  The first one makes it easy to comment on the
//			parameters, and is perhaps easier to read.
//
//			a. static int
//				integers_to_ascii_array
//				(
//					int firstone,
//					int secondone,
//						...
//				)
//				{
//					... define the function ...
//				}
//
//				All statement blocks should be indented in this manner,
//				too.
//
//			b. static int integers_to_ascii_array ( int firstone,
//					int secondone, ... )
//				{
//					... define the function ...
//				}
//				
//
// -------------------------------------------------
// Error Handling
// -------------------------------------------------
//
//		There are two main camps on the topic of error handling.  The first
// camp believes in using and checking return values.  The second camp
// believe in using exception handling.  Return values require great
// diligence in the codernaught, so that all values get checked.
// Exception handling requires great diligence in making constructors
// exception-safe (no leaks), and exceptions can generate some unwanted
// overhead.
//
//		Should VIDS have a single error-handling convention?  I don't
// know.  We should probably use both methods, depending on the module
// involved.
//
// -------------------------------------------------
// Other topics to discuss
// -------------------------------------------------
//
//	  1.	References versus pointers.
//	  2.	Makefiles versus IDEs.
//	  3.	Programmer's editors.
//	  4.	The Standard Template Library.
//	  5.	Library construction and module distribution.
//
//	Last.	The class functions declaration and sketched below.
//
//************************************************************************

#if !defined NiceClassCode_h
#define NiceClassCode_h

#include "U_class.h"				// U, a user-written or library class

typedef int B;						// B is a built-in type (e.g. integer)
typedef int T;						// T is any type; includes U or B


//************************************************************************
// Class member versions of declarations
//
//		Note the "friend" function.  Friend functions are a good way to
// extend the interface of a class without adding to the complexity of
// the class.  Also, overloaded operator functions need to be friends.
//
//		The rest of this class declaration shows the best function
// signatures for various member functions.  The functions themselves
// are fleshed out and described below.
//
//************************************************************************

class X : public U						// note the base class, U
{
	friend const X operator + (const X & x1, const X & x2);	// binary operator

public:

	X ();										// default constructor
	X (T t = 0);							// default constructor (default parameter)
	X (const X & x);						// copy constructor
	operator C ();							// conversion operator (inherited) named C
	operator C () const;					// conversion operator (safer)
	X & operator = (const X & x);		// principal assignment operator
	virtual ~X ();							// destructor (virtual for base classes)

	bool operator == (const X & x) const;	// equality operator
	bool operator != (const X & x) const;	// inequality operator

	int operator ! ();					// unary operator
	X & operator += (const X & x);	// assignment version of operator
	const X operator + (const X & x);// member version of binary operator
	X & operator ++ ();					// prefix increment operator
	const X operator ++ (int);			// postfix increment operator
	X & operator [] (T index) const;	// read subscript (must be non-static)
	X & operator [] (T index);			// write subscript (must be non-static)
	X operator () ();						// parenthesis operator
	Y * operator -> () const;			// indirection (dereference) operator
	Y & operator * () const;			// indirection operator

private:

	B built_in_type_member;
	T another_type_member;

};


//************************************************************************
// Global versions of declarations
//
//		Any of the following functions that need to be implemented in
// terms of private members of class X must be declared friends of
// class X, as shown above for
//
//			const X operator + (const X & x1, const X & x2);
//
//		Also note that there is a member version of operator +() declared
// above.   See the definition's comments way below for the reasoning.
//
//************************************************************************

bool operator == (const X & x1, const X & x2);		// equality operator
bool operator != (const X & x1, const X & x2);		// inequality operator
int operator ! (X & x);										// unary operator
const X operator + (const X & x1, const X & x2);	// binary operator
const X operator + (const X & x1, const T t);		// overloading on Tr
const X operator + (const T t, const X & x);			// commutativity


//************************************************************************
// IMPLEMENTATIONS OF CLASS MEMBERS
//************************************************************************
//************************************************************************
// Default constructor
//
//		1.	Be sure to include the member initializations in the order
//			of their declaration in the class.  The compiler will call them
//			in that same order, and you won't get confused in the debugger.
//
//		2.	Be very sure to make any constructed objects exception-safe
//			(e.g. by using the auto_ptr template class).  See "More
//			Effective C++" for details.  This safety requires great care
//			and a lot of work.
//
//		3.	Prefer initialization in the initializer list to assignment
//			in the body of the constructor; it's more efficient and
//			often more clear.
//
//		4.	Avoid writing code that calls the default constructor and
//			follows the call with a bunch of member initializations.
//
//			a.	Exposing the members publicly is dangerous.
//
//			b.	The code is less clear and definitely less efficient.
//				The caller is forced to maintain copies of member values
//				to use for the initialization.  [Microsoft's MFC violates
//				this advice a lot; Borland's OWL does a better job.]
//
//************************************************************************

inline
X::X ()
 :
	U( ... ),	// base class construction
					// member initializations
{
	// other code; Microsoft often assigns to members here, unfortunately
}


//************************************************************************
// Conversion via default constructor using default parameter
//************************************************************************

inline
X::X (T t = 0)
 :
	U( ... ),	// base class construction
					// member initializations
{
	// other code
}


//************************************************************************
// Copy constructor
//
//		1.	Consider implementing a reference-counting scheme.
//
//		2.	The copy constructor is a regular and nice function.
//
//		3.	Another valid form is X::X(const X & x, T t = 0)
//
//		Note that this does not cover the issue of copying the base class
// portion properly.  Not sure what to do, but see the principal
// assignment operator for an idea, using the = operator.  Also see
// Stroustrop's discussion of clone() or copy() functions.
//
//		It might be that the base classes appear first in the member
// initialization list, as in:
//
//		U(x)
//
// which should initialize the U part of the new X with the U part of the
// x parameter.
//
//		Note that we have two ways of detecting an error in a constructor
// call:
//
//		1.	Checking for a null pointer
//
//		2.	Catching a thrown exception.
//
//************************************************************************

inline
X::X (const X & x)
 :
	U( ... ),	// base class construction
	each_member		(x.each_member),
	.  .  .
{
	// Let xm be a pointer allocated in the constructor.  Then the following
	// stylized code could be used [many ways of copying can be employed]:

	xm = new Y;								// allocate space for the object
	if (xm != 0)							// make sure the allocation worked first
		*xm = *x.xm;						// copy the object
}


//************************************************************************
// Copy constructor (another version)
//
//		1.	Found this alternate method in Borland's OwlSock code.
//
//		2.	Note the use of operator =().
//
//		3.	This implementation mixes up the semantics of the copy
//			constructor and the principal assignment operator.  The
//			copy semantics are something like
//
//			a.	Create the object and its members.
//
//			b.	Allocate any memory, if applicable.
//
//			c.	Copy from the members of the source class, and, if applicable
//				from the memory allocated in the source class.
//
//			The assignment semantics are different, because the destination
//			already exists:
//
//			a.	Make sure the source and destination are not the same.  If the
//				same, skip to step e.
//
//			b.	Delete any allocated memory, if applicable.
//
//			c.	Re-allocate any new memory, if applicable.
//
//			d.	Copy from the members of the source class, and, if applicable
//				from the memory allocated in the source class.
//
//			e.	Return "*this".
//
//		4.	Do not write functions like those below.
//
//************************************************************************

inline
X::X (const X & x)
{
	operator =(x);			// bad semantics, and should cast to (void)
}

inline
X::X (const T & t)
{
	operator =(t);			// bad semantics, and should cast to (void)
}


//************************************************************************
// Principal assignment operator
//
//		1.	Must be a member function, to avoid absurdities.
//
//		2.	Must return a reference to X, to permit the chaining of
//			assignments.  [Scott Ladd's old book erroneously declared
//			this operator as "void".]
//
//		3.	Check to be sure that assignment to self is not done.
//
//			Might need to implement an identity scheme, especially when
//			multiple inheritance is involved, since there's no guarantee
//			that the address applies to the part of the object desired.
//			Here are the easy, but sometimes insufficient, alternatives:
//
//				if (this != &x)			// check pointer values [a fast method]
//				if (*this != x)			// check object value
//
//		3.	Not an inherited function.  Derived class's must provide
//			there own version.  The version provided by the compiler assigns
//			only the members of the derived class.
//
//		4.	Delete any existing resources in the object, if necessary.
//
//		5.	Note how the base class's operator can be called to increase
//			the consistency of the code, but only if the operator has been
//			explicitly declared.
//
//			Otherwise, a cast to a reference can be used.  Both alternatives
//			are shown below.
//
//		6.	Allocate any necessary resources and copy the values from the
//			source.
//
//			A copy() function can contain code common to both the copy
//			constructor and the principla assignment operator.  See
//			Stroustrop for advice.
//
//		7.	Assign the derived-class members.
//
//		8.	Return *this, always.
//
//************************************************************************

inline X &
X::operator = (const X & x)
{
	if (this != &x)						// make sure not assigning to self
	{

		// Let xm be a pointer allocated in the constructor.  Then the following
		// stylized code could be used:

		if (xm != 0)						// current object already have one?
			delete xm;						// yes, delete it (might need [] in some cases)

		#if base class U explicitly declares an assignment operator
			U::operator =(x);				// copy the U portion
		#else
			((U &) *this) = x;			// copy the U portion
		#endif

		each_member = x.each_member;	// copy the members

		xm = new Y;							// allocate space for the new object
		*xm = *x.xm;					   // copy the object
	}
	return *this;							// take note of this!
}


//************************************************************************
// Destructor
//
//		1.	Be sure to make the destructors of base classes virtual.  In this
//			way, the base class destructor will be called when the derived
//			class's destructor finishes.
//
//		2.	Even if a virtual destructor is pure, a definition must be
//			coded, since a base class's virtual destructor will be called
//			when the derived class's destructor finishes.
//
//		3.	Do not define a destructor unless one is necessary.  Otherwise
//			code is wasted, and the reader might be confused as to why
//			a do-nothing destructor is defined.
//
//		4.	"More Effective C++" discusses a method of controlling the
// 		propagation of exceptions, as hinted at in the code below.
//
//************************************************************************

inline
X::~X ()
{
	try
	{
		// code that might cause exceptions
	}
	catch (...)			// catch any exception
	{
		// do nothing; we just want to stop the exception from propagating further
	}
}


//************************************************************************
// Conversion operator (an inheritable function)
//
//		1.	The conversion operator is useful with smart pointers.
//
//		2.	Converts from a class to a basic type (e.g. a built-in type
//			or a structure).
//
//		3.	Warning:  The compiler can call the conversion implicitly.
//			Meyers recommends eschewing this operator.  Instead,
//			replace the built-in type T with a function name that has
//			less syntactic magic (e.g. "AsDouble" versus "double").
//
//			The keyword "explicit" can be used to avoid unintended
//			implicit conversions in compilers that are up to the latest
//			C++ specifications.
//
//		4.	This function is inherited by derived classes.
//
//		5.	Note the safer "const" version.
//
//		6.	It might be better to reinterpret_cast<T *>(this) below,
//
//************************************************************************

inline
X::operator T ()
{
	return *((T *) this);
}


inline
X::operator T () const					// the safer version
{
	return *((T *) this);
}


//************************************************************************
// Equality operator
//
//		1.	This is a regular and nice function.
//
//		2.	Use this operator to implement the operator !=().
//
//************************************************************************

inline bool
X::operator == (const X & x) const
{
	bool result = false;

	// implement the desired definition of equality.  For example,

	if (member == x.member)
		result = true;

	return result;
}


//************************************************************************
// Inequality operator
//
//		1.	This is a regular and nice function.
//
//		2.	Note how we use operator ==() to implement this operator.
//			This improves the correctability/maintainability of the code.
//
//************************************************************************

inline bool
X::operator != (const X & x) const
{
	return (*this == x) ? false : true ;	// implemented in terms of operator ==
}

//************************************************************************
// Less-than operator()
//
//		For containers, this is an important operator, because it not
// only defines the less-than operation, but containers defined the
// equality, inequality, greater-than, greater-than-or-equal, and
// less-than-or-equal functions in terms of less-than.
//
//		To make our comparison operators consistent with each other,
// following the discussion of Stroustrop, 3rd edition,
// section 17.1.4.1, we can define equivalence (equiv) in terms of our
// less-than comparison operator (cmp):
//
//			equiv(x, y) = not [ cmp(x, y) or cmp(y, x) ]
//
// It is easy to invert these to get the definition of operator !=()
// We don't have to worry, just define operator <.
//
//************************************************************************

inline bool
X::operator < (const X & x) const
{
	return key < x.key;
}


//************************************************************************
// Unary operator
//************************************************************************

inline int
X::operator ! ()
{
	// LATER, I'm getting tired
}


//************************************************************************
// Assignment version of binary operator
//
//		1.	Also applies to -=, /=, *=, etc.
//
//		2.	When writing the other operators based on this operator [that
//			is, operator +() and operator ++(), prefix and postfix
//			versions], implement them in terms of this operator, as shown
//			later, to improve the consistency of the code.
//
//		3.	When using the operators, note that the assignment version
//			of a binary operator is more efficient than the binary
//			operator (the "stand-alone" version of the operator).
//
//************************************************************************

inline X &
X::operator += (const X & x)
{
	X result;

	// implement the desired addition operation

	return result;
}


//************************************************************************
// Prefix increment operator
//
//		1.	Applies to operator --() also.
//
//		2.	Note that this implementation uses operator +=() for
//			consistency.
//
//		3.	Use the prefix version to implement the postfix version
//			(shown below).
//
//		4.	The prefix operator is more efficient than the postfix operator.
//
//************************************************************************

inline X &
X::operator ++ ()
{
	// One way to increment (if overloaded on a built-in integral type):

	*this += 1;								// (other forms possible)

	return *this;
}


//************************************************************************
// Postfix increment operator
//
//		1.	Applies to operator --() also.
//
//		2.	Note how we use the prefix version to implement this version to
//			promote consistency and maintainability.
//
//		3.	The prefix operator is more efficient than this operator.
//
//		4.	Note the "const" in the return value.  This prevents the
//			programmer from trying something like x++++, which is illegal
//			for integers, and would increment x only once anyway.
//
//		5.	The preceding precept illustrates that adage "when in doubt,
//			do as the ints do."
//
//************************************************************************

inline const X
X::operator ++ (int)
{
	X oldvalue = *this;					// save it for the return
	++(*this);								// use prefix increment
	return oldvalue;
}


//************************************************************************
// Subscript
//
//		1.	Must be a non-static member function.
//
//		2.	Note the two versions, the const one being for reads.  The
//			2nd edition of Scott Meyer's book makes the return value of
//			the read version a const.
//
//		3.	Designing a good subscript operator is a deep subject.  Refer
//			to Meyers' "More Effective C++".
//
//		4.	Rather than making your own array class, consider using the
//			STL vector class instead.
//
//************************************************************************

inline const X &
X::operator [] (T index) const
{
	// access code here
}


inline X &
X::operator [] (T index)
{
	// access code here
}


//************************************************************************
// Parenthesis operator
//
//		1.	Useful for making function objects.
//
//		2.	Again, a deep topic; see Meyers and see Stroustrop.
//
//************************************************************************

inline X
X::operator () ()
{
	// code needed here
}


//************************************************************************
// Indirection (dereference) operator
//
//		1.	Useful in dereferencing a smart pointer to get access to one
//			of its member functions.
//
//************************************************************************

inline Y *
X::operator -> () const
{
	// code needed here
}


//************************************************************************
// Indirection operator
//
//		1.	Useful in dereferencing a smart pointer.
//
//************************************************************************

inline Y &
X::operator * () const
{
	// code needed here
}


//************************************************************************
// Global versions
//
//		Some of the member functions above can also be written as
// global functions.  Sometimes this way of writing the function has
// advantages, such as (surprise!) improved encapsulation.
//
//		Many of the same comments as for their class-member brethren
// apply, and are not repeated here.
//
//************************************************************************

//************************************************************************
// Equality operator
//************************************************************************

inline bool
operator == (const X & x1, const X & x2)
{
	bool result;

	// implement the desired definition of equality

	return result;
}


//************************************************************************
// Inequality operator
//************************************************************************

inline bool
operator != (const X & x1, const X & x2)
{
	return (x1 == x2) ? false : true ;
}


//************************************************************************
// Unary operator
//************************************************************************

inline int
operator ! (X & x)
{
	// code needed here
}


//************************************************************************
// Binary operator (member version and global versions)
//
//		0.	The global version is preferable to the member version because
//			conversions can generally be made that support commutativity.
//
//		1.	Note the implementation in terms of the class version of
//			operator +=().
//
//			For example, assume that there exists a conversion from an
//			integer to a class X.  Then, given the declaration
//
//				X x(1);					// create an X instance
//
//			only one of the following is legal if operator +() is a member of
//			X:
//
//				X result = x + 2;		// fine, calls x.operator+(2)
//				X result = 2 + x;		// fails, there is no 2.operator+(const X &)
//
//		2.	Stroustrop prefers to implement operator +=() as a member of
//			the class X, then to implement operator +() as a global function,
//			as shown in the second version.
//
//		3.	Note the usage of the copy constructor in the return-value-
//			optimizable version.
//
//		4.	By not creating any named automatic (temporary) variables,
//			we give the compiler a chance for return-value optimization.
//			This could save constructor calls.
//
//		5.	The const return value avoids code such as x + + y.
//
//		6.	This function can be made efficient by keeping it defined
//			as an inline function.
//
//		7.	Also applies to -, /, and *, etc.
//
//		8.	Never perform this kind of overloading for operator &&(),
//			operator ||(), or operator ,() [the infamous comma operator].
//			For && and ||, short-circuit evaluation is not used in the
// 		overloaded versions, and this could cause confusion.   For
//			the comma operator, the behavior of "evaluate left expression
//			first, then right expression, and return the right expression"
//			cannot be mimicked in an overload.
//
//		9.	const X operator + (B b, B b) is illegal, because it would
//			change the built-in operator.  [Recall that we defined B as
//			a built-in type].
//
//	  10.	Meyers uses "lhs" and "rhs" (left- and right-hand side variable)
//			in place of "a" and "x", respectively).
//
//************************************************************************

inline const X
X::operator + (const X & x)				// first version
{
	return operator +=(x);					// calls this->operator+()
}

inline const X
operator + (const X & a, const X & x)	// second version, preferred
{
	X result = a;
	return a += x;								// calls a.operator+()
}

inline const X
operator + (const X & a, const X & x)	// return-value-optimizable version
{
	return X(a) += x;
}


//************************************************************************
// Overloading on T and Commutativity
//************************************************************************

inline const X
operator + (const X & x1, const T t)
{
	// code needed here
}

inline const X
operator + (const T t, const X & x)
{
	// code needed here
}


#endif	// NiceClassCode_h


