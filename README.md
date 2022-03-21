# Fox
A toy general purpose programming language that aims to be a mix of Zig and Rust without the borrow checker.

## How to Build
First download the source code from the repository.

Then run the premake file to generate your desired build/project files. For example:
```
premake5 gmake2
```

You should then be able to build the compiler with your usual tools.

## Language Feature List
- [x] Boolean values.
- [x] 64 bit integer and floating point values.
- [x] UTF8 string and character values.
- [x] Pointer values.
- [x] Arrays and slices.
- [x] Global and local variables.
- [ ] Compile-time constants. (partially implemented)
- [ ] First-class functions. (partially implemented)
- [ ] Variadic functions. (partially implemented)
- [x] Basic logical, relational and arithmetic operations.
- [x] Branching operations: ```if```, ```while```, ```for``` and ```match```.
- [ ] Pattern matching. (partially implemented)
- [x] Defer statements.
- [x] Structs.
- [x] Enums. 
- [x] Discriminated Unions.
- [ ] Traits. (partially implemented)
- [ ] Modules (partially implemented)
- [ ] Generics.

## Language Examples
### IntList
```
struct IntList {
	items: []mut int,
	count: int,
}

impl IntList {
	fn new() -> Self {
		return Self{ 
			items: []mut int{ 
				0 as *mut int, 
				0,
			}, 
			count: 0,
		};
	}

	fn with_capacity(cap: int) -> Self {
		return Self{ 
			items: []mut int{
				@alloc(*mut int, cap * @size_of(int)),
				cap,
			},
			count: 0,
		};
	}

	fn of(vararg ns: []int) -> Self {
		let mut self = Self::with_capacity(ns.len());
		for i in 0..ns.len() {
			self.items[i] = ns[i];
			self.count += 1;
		}
		return self;
	}

	fn free(mut self) {
		@free(self.items.data() as *void);
		self.items = []mut int{ 0 as *mut int, 0 };
		self.count = 0;
	}

	fn print(self) {
		@puts('[');
		for i in 0..self.count {
			@puts(self.items[i]);
			if i + 1 < self.count {
				@puts(", ");
			}
		}
		@print(']');
	}
}

let mut list = IntList::of(1, 2, 3, 4, 5);
defer list.free();

list.print();
```

### Shapes
```
enum Shape {
	Circle(float),
	Rectangle(float, float),
}

let shape = Shape::Circle(123.4);

match shape {
	Shape::Circle(radius) => {
		@puts("A circle with radius ");
		@print(radius);
	}
	Shape::Rectangle(w, h) => {
		@puts("A rectangle with a width of ");
		@puts(w)
		@puts(" and a height of ");
		@print(h);
	}
	_ => @panic("This should be impossible!");
}
```

### Functions and Tuples
```
fn divrem(a: int, b: int) -> (int, int) {
	return (a / b, a % b);
}

let tuple = divrem(9, 3);
let d = tuple.0;
let r = tuple.1;

// can destructure immediately
let (d, r) = divrem(9, 3);
```
