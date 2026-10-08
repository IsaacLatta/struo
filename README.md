# struo

## Custom parsers

Pass your parser directly to `struo::load<Config>(parser)`. The public
`struo::HasParserInterface<Parser>` concept checks this const interface:

```cpp
class MyParser {
public:
    template<typename T>
    struo::Result<T> getAs() const;

    struo::Result<std::optional<MyParser>> toChild(std::string_view key) const;
    struo::Result<std::vector<MyParser>> getElements() const;
    struo::Result<std::vector<std::pair<MyParser, MyParser>>> getMembers() const;
};

static_assert(struo::HasParserInterface<MyParser>);
```

`getAs<T>()` converts scalar nodes to the types used by your schema: arithmetic
values, strings, enums, filesystem paths, and chrono durations. The concept
checks representative types in each category; it cannot verify every template
specialization or the function bodies. `struo::HasGetAs<Parser, T>` checks a
specific conversion signature.

`toChild()` returns an empty optional for a missing key and an error for failed
navigation. `getElements()` returns sequence nodes; `getMembers()` returns pairs
of key and value nodes. Each child is another parser of the same type. Report
conversion and navigation failures through `struo::Result`; `load()` adds the
field or element path to errors. Any backing document must outlive the load.

Const parser references, mutable references, and temporary parsers are accepted.
