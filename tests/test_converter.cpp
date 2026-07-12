#include <pjh_cli.hpp>
#include <cassert>
#include <string_view>

int main()
{
    using pjh::cli::Converter;

    // int
    auto r1 =
        Converter<int>::
            from_string("42");
    assert(r1.is_ok());
    assert(r1.unwrap() == 42);

    auto r2 =
        Converter<int>::
            from_string("-10");
    assert(r2.is_ok());
    assert(r2.unwrap() == -10);

    auto r3 =
        Converter<int>::
            from_string("abc");
    assert(r3.is_err());

    // long
    auto r4 =
        Converter<long>::
            from_string("2147483647");
    assert(r4.is_ok());
    assert(r4.unwrap() == 2147483647L);

    // unsigned
    auto r5 =
        Converter<unsigned>::
            from_string("100");
    assert(r5.is_ok());
    assert(r5.unwrap() == 100u);

    auto r6 =
        Converter<unsigned>::
            from_string("-1");
    assert(r6.is_err());

    // bool
    auto r7 =
        Converter<bool>::
            from_string("true");
    assert(r7.is_ok() && r7.unwrap() == true);

    auto r8 =
        Converter<bool>::
            from_string("FALSE");
    assert(r8.is_ok() && r8.unwrap() == false);

    auto r9 =
        Converter<bool>::
            from_string("1");
    assert(r9.is_ok() && r9.unwrap() == true);

    auto r10 =
        Converter<bool>::
            from_string("yes");
    assert(r10.is_ok() && r10.unwrap() == true);

    auto r11 =
        Converter<bool>::
            from_string("n");
    assert(r11.is_ok() && r11.unwrap() == false);

    auto r12 =
        Converter<bool>::
            from_string("bad");
    assert(r12.is_err());

    // string
    auto r13 =
        Converter<std::string>::
            from_string("hello world");
    assert(r13.is_ok());
    assert(r13.unwrap() == "hello world");

    // float
    auto r14 =
        Converter<float>::
            from_string("3.14");
    assert(r14.is_ok());
    assert(r14.unwrap() > 3.13f && r14.unwrap() < 3.15f);

    auto r15 =
        Converter<float>::
            from_string("not_a_number");
    assert(r15.is_err());

    // double
    auto r16 =
        Converter<double>::
            from_string("2.71828");
    assert(r16.is_ok());
    assert(r16.unwrap() > 2.71 && r16.unwrap() < 2.72);

    // long long
    auto r17 =
        Converter<long long>::
            from_string("9999999999999");
    assert(r17.is_ok());
    assert(r17.unwrap() == 9999999999999LL);

    // unsigned long
    auto r18 =
        Converter<unsigned long>::
            from_string("3000000000");
    assert(r18.is_ok());

    // unsigned long long
    auto r19 =
        Converter<unsigned long long>::
            from_string("18446744073709551615");
    assert(r19.is_ok());

    return 0;
}
