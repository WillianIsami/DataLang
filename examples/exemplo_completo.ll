; DataLang - LLVM IR

; ==================== RUNTIME FUNCTIONS ====================

declare i32 @printf(i8*, ...)
declare i8* @malloc(i64)
declare void @free(i8*)
declare i32 @strcmp(i8*, i8*)

@.fmt.int = private constant [6 x i8] c"%lld\0A\00"
@.fmt.float = private constant [7 x i8] c"%0.6f\0A\00"
@.fmt.str = private constant [4 x i8] c"%s\0A\00"
@.fmt.true = private constant [6 x i8] c"true\0A\00"
@.fmt.false = private constant [7 x i8] c"false\0A\00"

@.fmt.int_no_nl = private constant [5 x i8] c"%lld\00"
@.fmt.float_no_nl = private constant [6 x i8] c"%0.6f\00"
@.fmt.str_no_nl = private constant [3 x i8] c"%s\00"
@.fmt.newline = private constant [2 x i8] c"\0A\00"
@.fmt.space = private constant [2 x i8] c" \00"
@.fmt.lbracket = private constant [2 x i8] c"[\00"
@.fmt.rbracket_nl = private constant [3 x i8] c"]\0A\00"
@.fmt.comma_space = private constant [3 x i8] c", \00"
@.fmt.bool_word_true = private constant [5 x i8] c"true\00"
@.fmt.bool_word_false = private constant [6 x i8] c"false\00"

; DataFrame operations
declare i8* @datalang_select(i8*, i32, ...)
declare i8* @datalang_groupby(i8*, i32, ...)
declare i64 @datalang_df_count(i8*)
declare i8* @datalang_df_filter_numeric(i8*, i8*, i32, double)
declare i8* @datalang_df_filter_string(i8*, i8*, i8*, i32)
declare {i64, double*} @datalang_df_column_double(i8*, i8*, double, double)
declare i8* @datalang_load(i8*)
declare void @datalang_save(i8*, i8*)
declare void @datalang_print_dataframe(i8*)
declare i8* @datalang_df_create(i32, ...)
declare void @datalang_df_add_row(i8*, i32, ...)
declare i8* @datalang_format_int(i64)
declare i8* @datalang_format_float(double)
declare i8* @datalang_format_bool(i1)
define void @print_int(i64 %val) {
  %fmt = getelementptr [6 x i8], [6 x i8]* @.fmt.int, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %fmt, i64 %val)
  ret void
}

define void @print_float(double %val) {
  %fmt = getelementptr [7 x i8], [7 x i8]* @.fmt.float, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %fmt, double %val)
  ret void
}

define void @print_string(i8* %val) {
  %fmt = getelementptr [4 x i8], [4 x i8]* @.fmt.str, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %fmt, i8* %val)
  ret void
}

define void @print_bool(i1 %val) {
  br i1 %val, label %is_true, label %is_false
is_true:
  %fmt_t = getelementptr [6 x i8], [6 x i8]* @.fmt.true, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %fmt_t)
  ret void
is_false:
  %fmt_f = getelementptr [7 x i8], [7 x i8]* @.fmt.false, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %fmt_f)
  ret void
}

define void @print_int_no_nl(i64 %val) {
  %fmt = getelementptr [5 x i8], [5 x i8]* @.fmt.int_no_nl, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %fmt, i64 %val)
  ret void
}

define void @print_float_no_nl(double %val) {
  %fmt = getelementptr [6 x i8], [6 x i8]* @.fmt.float_no_nl, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %fmt, double %val)
  ret void
}

define void @print_string_no_nl(i8* %val) {
  %fmt = getelementptr [3 x i8], [3 x i8]* @.fmt.str_no_nl, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %fmt, i8* %val)
  ret void
}

define void @print_bool_no_nl(i1 %val) {
  br i1 %val, label %is_true, label %is_false
is_true:
  %fmt_t = getelementptr [5 x i8], [5 x i8]* @.fmt.bool_word_true, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %fmt_t)
  ret void
is_false:
  %fmt_f = getelementptr [6 x i8], [6 x i8]* @.fmt.bool_word_false, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %fmt_f)
  ret void
}

define void @print_newline() {
  %fmt = getelementptr [2 x i8], [2 x i8]* @.fmt.newline, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %fmt)
  ret void
}

; print_int_array: imprime [1, 2, 3] em uma única linha
define void @print_int_array({i64, i64*} %array) {
entry:
  %size = extractvalue {i64, i64*} %array, 0
  %data = extractvalue {i64, i64*} %array, 1
  %fmt_lb = getelementptr [2 x i8], [2 x i8]* @.fmt.lbracket, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %fmt_lb)
  %i = alloca i64
  store i64 0, i64* %i
  br label %loop_cond
loop_cond:
  %i_val = load i64, i64* %i
  %cmp = icmp slt i64 %i_val, %size
  br i1 %cmp, label %loop_body, label %loop_end
loop_body:
  %ptr = getelementptr i64, i64* %data, i64 %i_val
  %val = load i64, i64* %ptr
  %fmt_elem = getelementptr [5 x i8], [5 x i8]* @.fmt.int_no_nl, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %fmt_elem, i64 %val)
  %i_next = add i64 %i_val, 1
  store i64 %i_next, i64* %i
  %has_next = icmp slt i64 %i_next, %size
  br i1 %has_next, label %print_sep, label %loop_cond
print_sep:
  %fmt_sep = getelementptr [3 x i8], [3 x i8]* @.fmt.comma_space, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %fmt_sep)
  br label %loop_cond
loop_end:
  %fmt_rb = getelementptr [3 x i8], [3 x i8]* @.fmt.rbracket_nl, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %fmt_rb)
  ret void
}

define void @print_float_array({i64, double*} %array) {
entry:
  %size = extractvalue {i64, double*} %array, 0
  %data = extractvalue {i64, double*} %array, 1
  %fmt_lb = getelementptr [2 x i8], [2 x i8]* @.fmt.lbracket, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %fmt_lb)
  %i = alloca i64
  store i64 0, i64* %i
  br label %loop_cond
loop_cond:
  %i_val = load i64, i64* %i
  %cmp = icmp slt i64 %i_val, %size
  br i1 %cmp, label %loop_body, label %loop_end
loop_body:
  %ptr = getelementptr double, double* %data, i64 %i_val
  %val = load double, double* %ptr
  %fmt_elem = getelementptr [6 x i8], [6 x i8]* @.fmt.float_no_nl, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %fmt_elem, double %val)
  %i_next = add i64 %i_val, 1
  store i64 %i_next, i64* %i
  %has_next = icmp slt i64 %i_next, %size
  br i1 %has_next, label %print_sep, label %loop_cond
print_sep:
  %fmt_sep = getelementptr [3 x i8], [3 x i8]* @.fmt.comma_space, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %fmt_sep)
  br label %loop_cond
loop_end:
  %fmt_rb = getelementptr [3 x i8], [3 x i8]* @.fmt.rbracket_nl, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %fmt_rb)
  ret void
}

define void @print_bool_array({i64, i1*} %array) {
entry:
  %size = extractvalue {i64, i1*} %array, 0
  %data = extractvalue {i64, i1*} %array, 1
  %fmt_lb = getelementptr [2 x i8], [2 x i8]* @.fmt.lbracket, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %fmt_lb)
  %i = alloca i64
  store i64 0, i64* %i
  br label %loop_cond
loop_cond:
  %i_val = load i64, i64* %i
  %cmp = icmp slt i64 %i_val, %size
  br i1 %cmp, label %loop_body, label %loop_end
loop_body:
  %ptr = getelementptr i1, i1* %data, i64 %i_val
  %val = load i1, i1* %ptr
  br i1 %val, label %print_true, label %print_false
print_true:
  %fmt_t = getelementptr [5 x i8], [5 x i8]* @.fmt.bool_word_true, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %fmt_t)
  br label %after_print
print_false:
  %fmt_f = getelementptr [6 x i8], [6 x i8]* @.fmt.bool_word_false, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %fmt_f)
  br label %after_print
after_print:
  %i_val2 = load i64, i64* %i
  %i_next = add i64 %i_val2, 1
  store i64 %i_next, i64* %i
  %has_next = icmp slt i64 %i_next, %size
  br i1 %has_next, label %print_sep, label %loop_cond
print_sep:
  %fmt_sep = getelementptr [3 x i8], [3 x i8]* @.fmt.comma_space, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %fmt_sep)
  br label %loop_cond
loop_end:
  %fmt_rb = getelementptr [3 x i8], [3 x i8]* @.fmt.rbracket_nl, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %fmt_rb)
  ret void
}

define void @print_string_array({i64, i8**} %array) {
entry:
  %size = extractvalue {i64, i8**} %array, 0
  %data = extractvalue {i64, i8**} %array, 1
  %fmt_lb = getelementptr [2 x i8], [2 x i8]* @.fmt.lbracket, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %fmt_lb)
  %i = alloca i64
  store i64 0, i64* %i
  br label %loop_cond
loop_cond:
  %i_val = load i64, i64* %i
  %cmp = icmp slt i64 %i_val, %size
  br i1 %cmp, label %loop_body, label %loop_end
loop_body:
  %ptr = getelementptr i8*, i8** %data, i64 %i_val
  %val = load i8*, i8** %ptr
  %fmt_elem = getelementptr [3 x i8], [3 x i8]* @.fmt.str_no_nl, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %fmt_elem, i8* %val)
  %i_next = add i64 %i_val, 1
  store i64 %i_next, i64* %i
  %has_next = icmp slt i64 %i_next, %size
  br i1 %has_next, label %print_sep, label %loop_cond
print_sep:
  %fmt_sep = getelementptr [3 x i8], [3 x i8]* @.fmt.comma_space, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %fmt_sep)
  br label %loop_cond
loop_end:
  %fmt_rb = getelementptr [3 x i8], [3 x i8]* @.fmt.rbracket_nl, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %fmt_rb)
  ret void
}

; sum: sums all elements in an integer array
define i64 @sum({i64, i64*} %array) {
entry:
  %size = extractvalue {i64, i64*} %array, 0
  %data = extractvalue {i64, i64*} %array, 1
  %size_zero = icmp eq i64 %size, 0
  br i1 %size_zero, label %return_zero, label %loop_cond
return_zero:
  ret i64 0
loop_cond:
  %i = phi i64 [0, %entry], [%i_next, %loop_body]
  %acc = phi i64 [0, %entry], [%acc_next, %loop_body]
  %cmp = icmp slt i64 %i, %size
  br i1 %cmp, label %loop_body, label %loop_end
loop_body:
  %ptr = getelementptr i64, i64* %data, i64 %i
  %val = load i64, i64* %ptr
  %acc_next = add i64 %acc, %val
  %i_next = add i64 %i, 1
  br label %loop_cond
loop_end:
  ret i64 %acc
}

; mean: calculates the average of an integer array, returns double
define double @mean({i64, i64*} %array) {
entry:
  %size = extractvalue {i64, i64*} %array, 0
  %data = extractvalue {i64, i64*} %array, 1
  %size_zero = icmp eq i64 %size, 0
  br i1 %size_zero, label %return_zero, label %loop_cond
return_zero:
  ret double 0.0
loop_cond:
  %i = phi i64 [0, %entry], [%i_next, %loop_body]
  %acc = phi i64 [0, %entry], [%acc_next, %loop_body]
  %cmp = icmp slt i64 %i, %size
  br i1 %cmp, label %loop_body, label %loop_end
loop_body:
  %ptr = getelementptr i64, i64* %data, i64 %i
  %val = load i64, i64* %ptr
  %acc_next = add i64 %acc, %val
  %i_next = add i64 %i, 1
  br label %loop_cond
loop_end:
  %sum_f = sitofp i64 %acc to double
  %size_f = sitofp i64 %size to double
  %result = fdiv double %sum_f, %size_f
  ret double %result
}

; count: returns the number of elements in an array
define i64 @count({i64, i64*} %array) {
  %size = extractvalue {i64, i64*} %array, 0
  ret i64 %size
}

; min: finds the minimum element in an integer array
define i64 @min({i64, i64*} %array) {
entry:
  %size = extractvalue {i64, i64*} %array, 0
  %data = extractvalue {i64, i64*} %array, 1
  %size_zero = icmp eq i64 %size, 0
  br i1 %size_zero, label %return_zero, label %init
return_zero:
  ret i64 0
init:
  %first_ptr = getelementptr i64, i64* %data, i64 0
  %first_val = load i64, i64* %first_ptr
  br label %loop_cond
loop_cond:
  %i = phi i64 [1, %init], [%i_next, %loop_body]
  %min_val = phi i64 [%first_val, %init], [%new_min, %loop_body]
  %cmp = icmp slt i64 %i, %size
  br i1 %cmp, label %loop_body, label %loop_end
loop_body:
  %ptr = getelementptr i64, i64* %data, i64 %i
  %val = load i64, i64* %ptr
  %is_less = icmp slt i64 %val, %min_val
  %new_min = select i1 %is_less, i64 %val, i64 %min_val
  %i_next = add i64 %i, 1
  br label %loop_cond
loop_end:
  ret i64 %min_val
}

; max: finds the maximum element in an integer array
define i64 @max({i64, i64*} %array) {
entry:
  %size = extractvalue {i64, i64*} %array, 0
  %data = extractvalue {i64, i64*} %array, 1
  %size_zero = icmp eq i64 %size, 0
  br i1 %size_zero, label %return_zero, label %init
return_zero:
  ret i64 0
init:
  %first_ptr = getelementptr i64, i64* %data, i64 0
  %first_val = load i64, i64* %first_ptr
  br label %loop_cond
loop_cond:
  %i = phi i64 [1, %init], [%i_next, %loop_body]
  %max_val = phi i64 [%first_val, %init], [%new_max, %loop_body]
  %cmp = icmp slt i64 %i, %size
  br i1 %cmp, label %loop_body, label %loop_end
loop_body:
  %ptr = getelementptr i64, i64* %data, i64 %i
  %val = load i64, i64* %ptr
  %is_greater = icmp sgt i64 %val, %max_val
  %new_max = select i1 %is_greater, i64 %val, i64 %max_val
  %i_next = add i64 %i, 1
  br label %loop_cond
loop_end:
  ret i64 %max_val
}

; sum_float: sums all elements in a float array
define double @sum_float({i64, double*} %array) {
entry:
  %size = extractvalue {i64, double*} %array, 0
  %data = extractvalue {i64, double*} %array, 1
  %size_zero = icmp eq i64 %size, 0
  br i1 %size_zero, label %return_zero, label %loop_cond
return_zero:
  ret double 0.0
loop_cond:
  %i = phi i64 [0, %entry], [%i_next, %loop_body]
  %acc = phi double [0.0, %entry], [%acc_next, %loop_body]
  %cmp = icmp slt i64 %i, %size
  br i1 %cmp, label %loop_body, label %loop_end
loop_body:
  %ptr = getelementptr double, double* %data, i64 %i
  %val = load double, double* %ptr
  %acc_next = fadd double %acc, %val
  %i_next = add i64 %i, 1
  br label %loop_cond
loop_end:
  ret double %acc
}

; mean_float: calculates the average of a float array
define double @mean_float({i64, double*} %array) {
entry:
  %size = extractvalue {i64, double*} %array, 0
  %data = extractvalue {i64, double*} %array, 1
  %size_zero = icmp eq i64 %size, 0
  br i1 %size_zero, label %return_zero, label %loop_cond
return_zero:
  ret double 0.0
loop_cond:
  %i = phi i64 [0, %entry], [%i_next, %loop_body]
  %acc = phi double [0.0, %entry], [%acc_next, %loop_body]
  %cmp = icmp slt i64 %i, %size
  br i1 %cmp, label %loop_body, label %loop_end
loop_body:
  %ptr = getelementptr double, double* %data, i64 %i
  %val = load double, double* %ptr
  %acc_next = fadd double %acc, %val
  %i_next = add i64 %i, 1
  br label %loop_cond
loop_end:
  %size_f = sitofp i64 %size to double
  %result = fdiv double %acc, %size_f
  ret double %result
}

; min_float: finds the minimum element in a float array
define double @min_float({i64, double*} %array) {
entry:
  %size = extractvalue {i64, double*} %array, 0
  %data = extractvalue {i64, double*} %array, 1
  %size_zero = icmp eq i64 %size, 0
  br i1 %size_zero, label %return_zero, label %init
return_zero:
  ret double 0.0
init:
  %first_ptr = getelementptr double, double* %data, i64 0
  %first_val = load double, double* %first_ptr
  br label %loop_cond
loop_cond:
  %i = phi i64 [1, %init], [%i_next, %loop_body]
  %min_val = phi double [%first_val, %init], [%new_min, %loop_body]
  %cmp = icmp slt i64 %i, %size
  br i1 %cmp, label %loop_body, label %loop_end
loop_body:
  %ptr = getelementptr double, double* %data, i64 %i
  %val = load double, double* %ptr
  %is_less = fcmp olt double %val, %min_val
  %new_min = select i1 %is_less, double %val, double %min_val
  %i_next = add i64 %i, 1
  br label %loop_cond
loop_end:
  ret double %min_val
}

; max_float: finds the maximum element in a float array
define double @max_float({i64, double*} %array) {
entry:
  %size = extractvalue {i64, double*} %array, 0
  %data = extractvalue {i64, double*} %array, 1
  %size_zero = icmp eq i64 %size, 0
  br i1 %size_zero, label %return_zero, label %init
return_zero:
  ret double 0.0
init:
  %first_ptr = getelementptr double, double* %data, i64 0
  %first_val = load double, double* %first_ptr
  br label %loop_cond
loop_cond:
  %i = phi i64 [1, %init], [%i_next, %loop_body]
  %max_val = phi double [%first_val, %init], [%new_max, %loop_body]
  %cmp = icmp slt i64 %i, %size
  br i1 %cmp, label %loop_body, label %loop_end
loop_body:
  %ptr = getelementptr double, double* %data, i64 %i
  %val = load double, double* %ptr
  %is_greater = fcmp ogt double %val, %max_val
  %new_max = select i1 %is_greater, double %val, double %max_val
  %i_next = add i64 %i, 1
  br label %loop_cond
loop_end:
  ret double %max_val
}

; ==================== DECLARAÇÕES ====================


; Data type: Person
%struct.Person = type { i8*, i64, i1 }

; Data type: Stats
%struct.Stats = type { {i64, i64*}, {i64, i64*}, {i64, i64*}, i8*, i8* }

; ==================== GLOBAL VARIABLES ====================

@global_globalInt = global i64 42
@global_globalFloat = global double 3.140000e+00
@global_globalBool = global i1 true
@global_globalString = global i8* getelementptr inbounds ([9 x i8], [9 x i8]* @.str.0, i32 0, i32 0)
@global_globalArray = global {i64, i64*} zeroinitializer

; Inicialização de variáveis globais
define void @__init_globals() {
entry:
  store i64 42, i64* @global_globalInt
  store double 3.140000000000000e+00, double* @global_globalFloat
  store i1 true, i1* @global_globalBool
  %t3 = getelementptr [9 x i8], [9 x i8]* @.str.0, i32 0, i32 0
  store i8* %t3, i8** @global_globalString
  %t4 = call i8* @malloc(i64 40)
  %t5 = bitcast i8* %t4 to i64*
  %t7 = getelementptr i64, i64* %t5, i64 0
  store i64 1, i64* %t7
  %t9 = getelementptr i64, i64* %t5, i64 1
  store i64 2, i64* %t9
  %t11 = getelementptr i64, i64* %t5, i64 2
  store i64 3, i64* %t11
  %t13 = getelementptr i64, i64* %t5, i64 3
  store i64 4, i64* %t13
  %t15 = getelementptr i64, i64* %t5, i64 4
  store i64 5, i64* %t15
  %t16 = alloca {i64, i64*}
  %t17 = getelementptr {i64, i64*}, {i64, i64*}* %t16, i32 0, i32 0
  store i64 5, i64* %t17
  %t18 = getelementptr {i64, i64*}, {i64, i64*}* %t16, i32 0, i32 1
  store i64* %t5, i64** %t18
  %t19 = load {i64, i64*}, {i64, i64*}* %t16
  store {i64, i64*} %t19, {i64, i64*}* @global_globalArray
  ret void
}

define void @log_test(i8* %p0, i64 %p1) {
entry:
  %t20 = alloca i8*
  store i8* %p0, i8** %t20
  %t21 = alloca i64
  store i64 %p1, i64* %t21
  %t22 = getelementptr [14 x i8], [14 x i8]* @.str.1, i32 0, i32 0
  call void @print_string(i8* %t22)
  %t23 = load i8*, i8** %t20
  call void @print_string(i8* %t23)
  %t24 = load i64, i64* %t21
  call void @print_int(i64 %t24)
  ret void
}

define i64 @test_literals_and_operators() {
entry:
  %t25 = alloca i64
  store i64 10, i64* %t25
  %t27 = alloca i64
  store i64 3, i64* %t27
  %t29 = alloca double
  store double 2.500000000000000e+00, double* %t29
  %t31 = alloca i1
  store i1 true, i1* %t31
  %t33 = alloca i1
  store i1 false, i1* %t33
  %t35 = alloca i8*
  %t36 = getelementptr [6 x i8], [6 x i8]* @.str.2, i32 0, i32 0
  store i8* %t36, i8** %t35
  %t37 = alloca double
  %t38 = load double, double* %t29
  %t40 = fmul double %t38, 2.000000000000000e+00
  store double %t40, double* %t37
  %t41 = alloca i1
  %t42 = load i8*, i8** %t35
  %t43 = getelementptr [6 x i8], [6 x i8]* @.str.2, i32 0, i32 0
  %t45 = icmp eq i8* %t42, null
  %t46 = icmp eq i8* %t43, null
  %t47 = and i1 %t45, %t46
  %t48 = or i1 %t45, %t46
  br i1 %t48, label %str_null_51, label %str_cmp_51
str_null_51:
  br label %str_merge_51
str_cmp_51:
  %t49 = call i32 @strcmp(i8* %t42, i8* %t43)
  %t50 = icmp eq i32 %t49, 0
  br label %str_merge_51
str_merge_51:
  %t44 = phi i1 [%t47, %str_null_51], [%t50, %str_cmp_51]
  store i1 %t44, i1* %t41
  %t52 = alloca i64
  %t53 = load i64, i64* %t25
  %t54 = load i64, i64* %t27
  %t55 = add i64 %t53, %t54
  store i64 %t55, i64* %t52
  %t56 = alloca i64
  %t57 = load i64, i64* %t25
  %t58 = load i64, i64* %t27
  %t59 = sub i64 %t57, %t58
  store i64 %t59, i64* %t56
  %t60 = alloca i64
  %t61 = load i64, i64* %t25
  %t62 = load i64, i64* %t27
  %t63 = mul i64 %t61, %t62
  store i64 %t63, i64* %t60
  %t64 = alloca i64
  %t65 = load i64, i64* %t25
  %t66 = load i64, i64* %t27
  %t67 = sdiv i64 %t65, %t66
  store i64 %t67, i64* %t64
  %t68 = alloca i64
  %t69 = load i64, i64* %t25
  %t70 = load i64, i64* %t27
  %t71 = srem i64 %t69, %t70
  store i64 %t71, i64* %t68
  %t72 = alloca i1
  %t73 = load i64, i64* %t25
  %t74 = load i64, i64* %t27
  %t75 = icmp sgt i64 %t73, %t74
  %t76 = load i64, i64* %t27
  %t78 = icmp sge i64 %t76, 0
  %t79 = and i1 %t75, %t78
  store i1 %t79, i1* %t72
  %t80 = alloca i1
  %t81 = load i64, i64* %t25
  %t83 = icmp eq i64 %t81, 10
  %t84 = load i64, i64* %t27
  %t86 = icmp eq i64 %t84, 0
  %t87 = or i1 %t83, %t86
  store i1 %t87, i1* %t80
  %t88 = alloca i1
  %t89 = load i64, i64* %t25
  %t90 = load i64, i64* %t27
  %t91 = icmp slt i64 %t89, %t90
  %t92 = xor i1 %t91, true
  store i1 %t92, i1* %t88
  %t93 = alloca i64
  store i64 0, i64* %t93
  %t95 = alloca i64
  store i64 0, i64* %t95
  store i64 7, i64* %t95
  store i64 7, i64* %t93
  %t98 = load i64, i64* %t52
  %t100 = icmp eq i64 %t98, 13
  %t101 = load i64, i64* %t56
  %t103 = icmp eq i64 %t101, 7
  %t104 = and i1 %t100, %t103
  %t105 = load i64, i64* %t60
  %t107 = icmp eq i64 %t105, 30
  %t108 = and i1 %t104, %t107
  %t109 = load i64, i64* %t64
  %t111 = icmp eq i64 %t109, 3
  %t112 = and i1 %t108, %t111
  %t113 = load i64, i64* %t68
  %t115 = icmp eq i64 %t113, 1
  %t116 = and i1 %t112, %t115
  %t117 = load i1, i1* %t72
  %t119 = icmp eq i1 %t117, true
  %t120 = and i1 %t116, %t119
  %t121 = load i1, i1* %t80
  %t123 = icmp eq i1 %t121, true
  %t124 = and i1 %t120, %t123
  %t125 = load i1, i1* %t88
  %t127 = icmp eq i1 %t125, true
  %t128 = and i1 %t124, %t127
  %t129 = load i64, i64* %t93
  %t131 = icmp eq i64 %t129, 7
  %t132 = and i1 %t128, %t131
  %t133 = load i64, i64* %t95
  %t135 = icmp eq i64 %t133, 7
  %t136 = and i1 %t132, %t135
  %t137 = load i1, i1* %t31
  %t139 = icmp eq i1 %t137, true
  %t140 = and i1 %t136, %t139
  %t141 = load i1, i1* %t33
  %t143 = icmp eq i1 %t141, false
  %t144 = and i1 %t140, %t143
  %t145 = load double, double* %t37
  %t147 = fcmp oeq double %t145, 5.000000000000000e+00
  %t148 = and i1 %t144, %t147
  %t149 = load i1, i1* %t41
  %t151 = icmp eq i1 %t149, true
  %t152 = and i1 %t148, %t151
  br i1 %t152, label %L0, label %L1

L0:
  ret i64 0
  br label %L2

L1:
  ret i64 1
  br label %L2

L2:
  ret i64 0
}

define i64 @test_data_and_postfix() {
entry:
  %t155 = alloca %struct.Person*
  %t156 = call i8* @malloc(i64 24)
  %t157 = bitcast i8* %t156 to %struct.Person*
  %t158 = getelementptr [6 x i8], [6 x i8]* @.str.3, i32 0, i32 0
  %t159 = getelementptr %struct.Person, %struct.Person* %t157, i32 0, i32 0
  store i8* %t158, i8** %t159
  %t161 = getelementptr %struct.Person, %struct.Person* %t157, i32 0, i32 1
  store i64 30, i64* %t161
  %t163 = getelementptr %struct.Person, %struct.Person* %t157, i32 0, i32 2
  store i1 true, i1* %t163
  store %struct.Person* %t157, %struct.Person** %t155
  %t164 = alloca i8*
  %t165 = load %struct.Person*, %struct.Person** %t155
  %t166 = getelementptr %struct.Person, %struct.Person* %t165, i32 0, i32 0
  %t167 = load i8*, i8** %t166
  store i8* %t167, i8** %t164
  %t168 = alloca i64
  %t169 = load %struct.Person*, %struct.Person** %t155
  %t170 = getelementptr %struct.Person, %struct.Person* %t169, i32 0, i32 1
  %t171 = load i64, i64* %t170
  store i64 %t171, i64* %t168
  %t172 = alloca i1
  %t173 = load %struct.Person*, %struct.Person** %t155
  %t174 = getelementptr %struct.Person, %struct.Person* %t173, i32 0, i32 2
  %t175 = load i1, i1* %t174
  store i1 %t175, i1* %t172
  %t176 = alloca {i64, i64*}
  %t177 = call i8* @malloc(i64 24)
  %t178 = bitcast i8* %t177 to i64*
  %t180 = getelementptr i64, i64* %t178, i64 0
  store i64 10, i64* %t180
  %t182 = getelementptr i64, i64* %t178, i64 1
  store i64 20, i64* %t182
  %t184 = getelementptr i64, i64* %t178, i64 2
  store i64 30, i64* %t184
  %t185 = alloca {i64, i64*}
  %t186 = getelementptr {i64, i64*}, {i64, i64*}* %t185, i32 0, i32 0
  store i64 3, i64* %t186
  %t187 = getelementptr {i64, i64*}, {i64, i64*}* %t185, i32 0, i32 1
  store i64* %t178, i64** %t187
  %t188 = load {i64, i64*}, {i64, i64*}* %t185
  store {i64, i64*} %t188, {i64, i64*}* %t176
  %t189 = alloca i64
  %t190 = load {i64, i64*}, {i64, i64*}* %t176
  %t192 = extractvalue {i64, i64*} %t190, 1
  %t193 = getelementptr i64, i64* %t192, i64 0
  %t194 = load i64, i64* %t193
  store i64 %t194, i64* %t189
  %t195 = alloca i64
  %t196 = load {i64, i64*}, {i64, i64*}* %t176
  %t198 = extractvalue {i64, i64*} %t196, 1
  %t199 = getelementptr i64, i64* %t198, i64 1
  %t200 = load i64, i64* %t199
  store i64 %t200, i64* %t195
  %t201 = getelementptr [28 x i8], [28 x i8]* @.str.4, i32 0, i32 0
  call void @print_string(i8* %t201)
  %t202 = load i8*, i8** %t164
  call void @print_string(i8* %t202)
  %t203 = load i64, i64* %t168
  call void @print_int(i64 %t203)
  %t204 = load i64, i64* %t189
  call void @print_int(i64 %t204)
  %t205 = load i64, i64* %t195
  call void @print_int(i64 %t205)
  %t206 = load i8*, i8** %t164
  %t207 = getelementptr [6 x i8], [6 x i8]* @.str.3, i32 0, i32 0
  %t209 = icmp eq i8* %t206, null
  %t210 = icmp eq i8* %t207, null
  %t211 = and i1 %t209, %t210
  %t212 = or i1 %t209, %t210
  br i1 %t212, label %str_null_215, label %str_cmp_215
str_null_215:
  br label %str_merge_215
str_cmp_215:
  %t213 = call i32 @strcmp(i8* %t206, i8* %t207)
  %t214 = icmp eq i32 %t213, 0
  br label %str_merge_215
str_merge_215:
  %t208 = phi i1 [%t211, %str_null_215], [%t214, %str_cmp_215]
  %t216 = load i64, i64* %t168
  %t218 = icmp eq i64 %t216, 30
  %t219 = and i1 %t208, %t218
  %t220 = load i1, i1* %t172
  %t222 = icmp eq i1 %t220, true
  %t223 = and i1 %t219, %t222
  %t224 = load i64, i64* %t189
  %t226 = icmp eq i64 %t224, 10
  %t227 = and i1 %t223, %t226
  %t228 = load i64, i64* %t195
  %t230 = icmp eq i64 %t228, 20
  %t231 = and i1 %t227, %t230
  br i1 %t231, label %L3, label %L4

L3:
  ret i64 0
  br label %L5

L4:
  ret i64 1
  br label %L5

L5:
  ret i64 0
}

define i64 @test_if_else(i64 %p0) {
entry:
  %t234 = alloca i64
  store i64 %p0, i64* %t234
  %t235 = alloca i64
  store i64 0, i64* %t235
  %t237 = load i64, i64* %t234
  %t239 = icmp slt i64 %t237, 0
  br i1 %t239, label %L6, label %L7

L6:
  %t240 = load i64, i64* %t234
  %t242 = icmp slt i64 %t240, -100
  br i1 %t242, label %L9, label %L11

L9:
  %t243 = getelementptr [15 x i8], [15 x i8]* @.str.5, i32 0, i32 0
  call void @print_string(i8* %t243)
  br label %L11

L11:
  %t244 = alloca i64
  store i64 -1, i64* %t244
  %t246 = load i64, i64* %t244
  %t248 = icmp eq i64 %t246, -1
  br i1 %t248, label %L12, label %L14

L12:
  %t249 = load i64, i64* %t235
  %t251 = add i64 %t249, 0
  store i64 %t251, i64* %t235
  br label %L14

L14:
  %t252 = alloca i64
  store i64 -1, i64* %t252
  %t254 = load i64, i64* %t252
  %t256 = icmp eq i64 %t254, -1
  br i1 %t256, label %L15, label %L17

L15:
  %t257 = load i64, i64* %t235
  %t259 = add i64 %t257, 0
  store i64 %t259, i64* %t235
  br label %L17

L17:
  ret i64 0
  br label %L8

L7:
  %t261 = load i64, i64* %t234
  %t263 = icmp eq i64 %t261, 0
  br i1 %t263, label %L18, label %L19

L18:
  ret i64 0
  br label %L20

L19:
  ret i64 0
  br label %L20

L20:
  br label %L8

L8:
  %t266 = load i64, i64* %t235
  ret i64 %t266
}

define i64 @test_for_and_range() {
entry:
  %t267 = alloca i64
  store i64 0, i64* %t267
  %t271 = sub i64 5, 1
  %t272 = add i64 %t271, 1
  %t273 = mul i64 %t272, 8
  %t274 = call i8* @malloc(i64 %t273)
  %t275 = bitcast i8* %t274 to i64*
  %t276 = alloca i64
  store i64 0, i64* %t276
  %t277 = alloca i64
  store i64 1, i64* %t277
  br label %L21

L21:
  %t278 = load i64, i64* %t276
  %t279 = icmp slt i64 %t278, %t272
  br i1 %t279, label %L22, label %L23

L22:
  %t280 = load i64, i64* %t277
  %t281 = getelementptr i64, i64* %t275, i64 %t278
  store i64 %t280, i64* %t281
  %t282 = add i64 %t280, 1
  store i64 %t282, i64* %t277
  %t283 = add i64 %t278, 1
  store i64 %t283, i64* %t276
  br label %L21

L23:
  %t284 = alloca {i64, i64*}
  %t285 = getelementptr {i64, i64*}, {i64, i64*}* %t284, i32 0, i32 0
  store i64 %t272, i64* %t285
  %t286 = getelementptr {i64, i64*}, {i64, i64*}* %t284, i32 0, i32 1
  store i64* %t275, i64** %t286
  %t287 = load {i64, i64*}, {i64, i64*}* %t284
  %t288 = extractvalue {i64, i64*} %t287, 0
  %t289 = extractvalue {i64, i64*} %t287, 1
  %t290 = alloca i64
  store i64 0, i64* %t290
  %t291 = alloca i64
  br label %L24

L24:
  %t292 = load i64, i64* %t290
  %t293 = icmp slt i64 %t292, %t288
  br i1 %t293, label %L25, label %L26

L25:
  %t294 = getelementptr i64, i64* %t289, i64 %t292
  %t295 = load i64, i64* %t294
  store i64 %t295, i64* %t291
  %t296 = load i64, i64* %t267
  %t297 = load i64, i64* %t291
  %t298 = add i64 %t296, %t297
  store i64 %t298, i64* %t267
  %t299 = add i64 %t292, 1
  store i64 %t299, i64* %t290
  br label %L24

L26:
  %t300 = alloca i64
  store i64 0, i64* %t300
  %t302 = alloca {i64, i64*}
  %t303 = call i8* @malloc(i64 32)
  %t304 = bitcast i8* %t303 to i64*
  %t306 = getelementptr i64, i64* %t304, i64 0
  store i64 1, i64* %t306
  %t308 = getelementptr i64, i64* %t304, i64 1
  store i64 2, i64* %t308
  %t310 = getelementptr i64, i64* %t304, i64 2
  store i64 3, i64* %t310
  %t312 = getelementptr i64, i64* %t304, i64 3
  store i64 4, i64* %t312
  %t313 = alloca {i64, i64*}
  %t314 = getelementptr {i64, i64*}, {i64, i64*}* %t313, i32 0, i32 0
  store i64 4, i64* %t314
  %t315 = getelementptr {i64, i64*}, {i64, i64*}* %t313, i32 0, i32 1
  store i64* %t304, i64** %t315
  %t316 = load {i64, i64*}, {i64, i64*}* %t313
  store {i64, i64*} %t316, {i64, i64*}* %t302
  %t317 = load {i64, i64*}, {i64, i64*}* %t302
  %t318 = extractvalue {i64, i64*} %t317, 0
  %t319 = extractvalue {i64, i64*} %t317, 1
  %t320 = alloca i64
  store i64 0, i64* %t320
  %t321 = alloca i64
  br label %L27

L27:
  %t322 = load i64, i64* %t320
  %t323 = icmp slt i64 %t322, %t318
  br i1 %t323, label %L28, label %L29

L28:
  %t324 = getelementptr i64, i64* %t319, i64 %t322
  %t325 = load i64, i64* %t324
  store i64 %t325, i64* %t321
  %t326 = load i64, i64* %t300
  %t327 = load i64, i64* %t321
  %t328 = add i64 %t326, %t327
  store i64 %t328, i64* %t300
  %t329 = add i64 %t322, 1
  store i64 %t329, i64* %t320
  br label %L27

L29:
  %t330 = load i64, i64* %t267
  %t332 = icmp eq i64 %t330, 15
  %t333 = load i64, i64* %t300
  %t335 = icmp eq i64 %t333, 10
  %t336 = and i1 %t332, %t335
  br i1 %t336, label %L30, label %L31

L30:
  ret i64 0
  br label %L32

L31:
  ret i64 1
  br label %L32

L32:
  ret i64 0
}

define i64 @test_lambdas_and_pipelines() {
entry:
  %t339 = alloca {i64, i64*}
  %t340 = call i8* @malloc(i64 48)
  %t341 = bitcast i8* %t340 to i64*
  %t343 = getelementptr i64, i64* %t341, i64 0
  store i64 1, i64* %t343
  %t345 = getelementptr i64, i64* %t341, i64 1
  store i64 2, i64* %t345
  %t347 = getelementptr i64, i64* %t341, i64 2
  store i64 3, i64* %t347
  %t349 = getelementptr i64, i64* %t341, i64 3
  store i64 4, i64* %t349
  %t351 = getelementptr i64, i64* %t341, i64 4
  store i64 5, i64* %t351
  %t353 = getelementptr i64, i64* %t341, i64 5
  store i64 6, i64* %t353
  %t354 = alloca {i64, i64*}
  %t355 = getelementptr {i64, i64*}, {i64, i64*}* %t354, i32 0, i32 0
  store i64 6, i64* %t355
  %t356 = getelementptr {i64, i64*}, {i64, i64*}* %t354, i32 0, i32 1
  store i64* %t341, i64** %t356
  %t357 = load {i64, i64*}, {i64, i64*}* %t354
  store {i64, i64*} %t357, {i64, i64*}* %t339
  %t358 = alloca i64
  %t359 = load {i64, i64*}, {i64, i64*}* %t339
  %t360 = extractvalue {i64, i64*} %t359, 0
  %t361 = extractvalue {i64, i64*} %t359, 1
  %t362 = mul i64 %t360, 8
  %t363 = call i8* @malloc(i64 %t362)
  %t364 = bitcast i8* %t363 to i64*
  %t365 = alloca i64
  store i64 0, i64* %t365
  %t366 = alloca i64
  store i64 0, i64* %t366
  br label %L33

L33:
  %t367 = load i64, i64* %t365
  %t368 = icmp slt i64 %t367, %t360
  br i1 %t368, label %L34, label %L37

L34:
  %t369 = getelementptr i64, i64* %t361, i64 %t367
  %t370 = load i64, i64* %t369
  %t371 = alloca i64
  store i64 %t370, i64* %t371
  %t372 = load i64, i64* %t371
  %t374 = srem i64 %t372, 2
  %t376 = icmp eq i64 %t374, 0
  br i1 %t376, label %L35, label %L36

L35:
  %t377 = load i64, i64* %t366
  %t378 = getelementptr i64, i64* %t364, i64 %t377
  store i64 %t370, i64* %t378
  %t379 = add i64 %t377, 1
  store i64 %t379, i64* %t366
  br label %L36

L36:
  %t380 = add i64 %t367, 1
  store i64 %t380, i64* %t365
  br label %L33

L37:
  %t381 = load i64, i64* %t366
  %t382 = alloca {i64, i64*}
  %t383 = getelementptr {i64, i64*}, {i64, i64*}* %t382, i32 0, i32 0
  store i64 %t381, i64* %t383
  %t384 = getelementptr {i64, i64*}, {i64, i64*}* %t382, i32 0, i32 1
  store i64* %t364, i64** %t384
  %t385 = load {i64, i64*}, {i64, i64*}* %t382
  %t386 = extractvalue {i64, i64*} %t385, 0
  %t387 = extractvalue {i64, i64*} %t385, 1
  %t388 = mul i64 %t386, 8
  %t389 = call i8* @malloc(i64 %t388)
  %t390 = bitcast i8* %t389 to i64*
  %t391 = alloca i64
  store i64 0, i64* %t391
  br label %L38

L38:
  %t392 = load i64, i64* %t391
  %t393 = icmp slt i64 %t392, %t386
  br i1 %t393, label %L39, label %L40

L39:
  %t394 = getelementptr i64, i64* %t387, i64 %t392
  %t395 = load i64, i64* %t394
  %t396 = alloca i64
  store i64 %t395, i64* %t396
  %t397 = load i64, i64* %t396
  %t398 = load i64, i64* %t396
  %t399 = mul i64 %t397, %t398
  %t400 = getelementptr i64, i64* %t390, i64 %t392
  store i64 %t399, i64* %t400
  %t401 = add i64 %t392, 1
  store i64 %t401, i64* %t391
  br label %L38

L40:
  %t402 = alloca {i64, i64*}
  %t403 = getelementptr {i64, i64*}, {i64, i64*}* %t402, i32 0, i32 0
  store i64 %t386, i64* %t403
  %t404 = getelementptr {i64, i64*}, {i64, i64*}* %t402, i32 0, i32 1
  store i64* %t390, i64** %t404
  %t405 = load {i64, i64*}, {i64, i64*}* %t402
  %t406 = extractvalue {i64, i64*} %t405, 0
  %t407 = extractvalue {i64, i64*} %t405, 1
  %t409 = alloca i64
  store i64 0, i64* %t409
  %t410 = alloca i64
  store i64 0, i64* %t410
  br label %L41

L41:
  %t411 = load i64, i64* %t410
  %t412 = icmp slt i64 %t411, %t406
  br i1 %t412, label %L42, label %L43

L42:
  %t413 = load i64, i64* %t409
  %t414 = getelementptr i64, i64* %t407, i64 %t411
  %t415 = load i64, i64* %t414
  %t416 = add i64 %t413, %t415
  store i64 %t416, i64* %t409
  %t417 = add i64 %t411, 1
  store i64 %t417, i64* %t410
  br label %L41

L43:
  %t418 = load i64, i64* %t409
  store i64 %t418, i64* %t358
  %t419 = load i64, i64* %t358
  %t421 = icmp ne i64 %t419, 56
  br i1 %t421, label %L44, label %L46

L44:
  ret i64 1
  br label %L46

L46:
  %t423 = alloca double
  %t424 = load {i64, i64*}, {i64, i64*}* %t339
  %t425 = extractvalue {i64, i64*} %t424, 0
  %t426 = extractvalue {i64, i64*} %t424, 1
  %t427 = mul i64 %t425, 8
  %t428 = call i8* @malloc(i64 %t427)
  %t429 = bitcast i8* %t428 to i64*
  %t430 = alloca i64
  store i64 0, i64* %t430
  %t431 = alloca i64
  store i64 0, i64* %t431
  br label %L47

L47:
  %t432 = load i64, i64* %t430
  %t433 = icmp slt i64 %t432, %t425
  br i1 %t433, label %L48, label %L51

L48:
  %t434 = getelementptr i64, i64* %t426, i64 %t432
  %t435 = load i64, i64* %t434
  %t436 = alloca i64
  store i64 %t435, i64* %t436
  %t437 = load i64, i64* %t436
  %t439 = srem i64 %t437, 2
  %t441 = icmp eq i64 %t439, 0
  br i1 %t441, label %L49, label %L50

L49:
  %t442 = load i64, i64* %t431
  %t443 = getelementptr i64, i64* %t429, i64 %t442
  store i64 %t435, i64* %t443
  %t444 = add i64 %t442, 1
  store i64 %t444, i64* %t431
  br label %L50

L50:
  %t445 = add i64 %t432, 1
  store i64 %t445, i64* %t430
  br label %L47

L51:
  %t446 = load i64, i64* %t431
  %t447 = alloca {i64, i64*}
  %t448 = getelementptr {i64, i64*}, {i64, i64*}* %t447, i32 0, i32 0
  store i64 %t446, i64* %t448
  %t449 = getelementptr {i64, i64*}, {i64, i64*}* %t447, i32 0, i32 1
  store i64* %t429, i64** %t449
  %t450 = load {i64, i64*}, {i64, i64*}* %t447
  %t451 = call double @mean({i64, i64*} %t450)
  store double %t451, double* %t423
  %t452 = alloca double
  %t453 = load {i64, i64*}, {i64, i64*}* %t339
  %t454 = call double @mean({i64, i64*} %t453)
  store double %t454, double* %t452
  %t455 = getelementptr [17 x i8], [17 x i8]* @.str.6, i32 0, i32 0
  call void @print_string(i8* %t455)
  %t456 = load double, double* %t423
  call void @print_float(double %t456)
  %t457 = getelementptr [13 x i8], [13 x i8]* @.str.7, i32 0, i32 0
  call void @print_string(i8* %t457)
  %t458 = load double, double* %t452
  call void @print_float(double %t458)
  ret i64 0
}

define i64 @test_aggregates() {
entry:
  %t460 = alloca {i64, i64*}
  %t461 = call i8* @malloc(i64 40)
  %t462 = bitcast i8* %t461 to i64*
  %t464 = getelementptr i64, i64* %t462, i64 0
  store i64 1, i64* %t464
  %t466 = getelementptr i64, i64* %t462, i64 1
  store i64 2, i64* %t466
  %t468 = getelementptr i64, i64* %t462, i64 2
  store i64 3, i64* %t468
  %t470 = getelementptr i64, i64* %t462, i64 3
  store i64 4, i64* %t470
  %t472 = getelementptr i64, i64* %t462, i64 4
  store i64 5, i64* %t472
  %t473 = alloca {i64, i64*}
  %t474 = getelementptr {i64, i64*}, {i64, i64*}* %t473, i32 0, i32 0
  store i64 5, i64* %t474
  %t475 = getelementptr {i64, i64*}, {i64, i64*}* %t473, i32 0, i32 1
  store i64* %t462, i64** %t475
  %t476 = load {i64, i64*}, {i64, i64*}* %t473
  store {i64, i64*} %t476, {i64, i64*}* %t460
  %t477 = alloca i64
  %t478 = load {i64, i64*}, {i64, i64*}* %t460
  %t479 = call i64 @sum({i64, i64*} %t478)
  store i64 %t479, i64* %t477
  %t480 = alloca i64
  %t481 = load {i64, i64*}, {i64, i64*}* %t460
  %t482 = call i64 @count({i64, i64*} %t481)
  store i64 %t482, i64* %t480
  %t483 = alloca i64
  %t484 = load {i64, i64*}, {i64, i64*}* %t460
  %t485 = call i64 @min({i64, i64*} %t484)
  store i64 %t485, i64* %t483
  %t486 = alloca i64
  %t487 = load {i64, i64*}, {i64, i64*}* %t460
  %t488 = call i64 @max({i64, i64*} %t487)
  store i64 %t488, i64* %t486
  %t489 = load i64, i64* %t477
  %t491 = icmp eq i64 %t489, 15
  %t492 = load i64, i64* %t480
  %t494 = icmp eq i64 %t492, 5
  %t495 = and i1 %t491, %t494
  %t496 = load i64, i64* %t483
  %t498 = icmp eq i64 %t496, 1
  %t499 = and i1 %t495, %t498
  %t500 = load i64, i64* %t486
  %t502 = icmp eq i64 %t500, 5
  %t503 = and i1 %t499, %t502
  br i1 %t503, label %L52, label %L53

L52:
  ret i64 0
  br label %L54

L53:
  ret i64 1
  br label %L54

L54:
  ret i64 0
}

define i64 @test_dataframe_pipeline() {
entry:
  %t506 = getelementptr [50 x i8], [50 x i8]* @.str.8, i32 0, i32 0
  call void @print_string(i8* %t506)
  ret i64 0
}

define i64 @test_assignment_and_logic() {
entry:
  %t508 = alloca i64
  store i64 0, i64* %t508
  %t510 = alloca i64
  store i64 0, i64* %t510
  store i64 10, i64* %t510
  store i64 10, i64* %t508
  %t513 = alloca i1
  %t514 = load i64, i64* %t508
  %t516 = icmp eq i64 %t514, 10
  %t517 = load i64, i64* %t510
  %t519 = icmp eq i64 %t517, 10
  %t520 = and i1 %t516, %t519
  %t522 = or i1 %t520, false
  store i1 %t522, i1* %t513
  %t523 = load i1, i1* %t513
  %t524 = xor i1 %t523, true
  br i1 %t524, label %L55, label %L57

L55:
  ret i64 1
  br label %L57

L57:
  ret i64 0
}

define i64 @test_lambda_variable() {
entry:
  %t527 = alloca {i64, i64*}
  %t528 = call i8* @malloc(i64 40)
  %t529 = bitcast i8* %t528 to i64*
  %t531 = getelementptr i64, i64* %t529, i64 0
  store i64 1, i64* %t531
  %t533 = getelementptr i64, i64* %t529, i64 1
  store i64 2, i64* %t533
  %t535 = getelementptr i64, i64* %t529, i64 2
  store i64 3, i64* %t535
  %t537 = getelementptr i64, i64* %t529, i64 3
  store i64 4, i64* %t537
  %t539 = getelementptr i64, i64* %t529, i64 4
  store i64 5, i64* %t539
  %t540 = alloca {i64, i64*}
  %t541 = getelementptr {i64, i64*}, {i64, i64*}* %t540, i32 0, i32 0
  store i64 5, i64* %t541
  %t542 = getelementptr {i64, i64*}, {i64, i64*}* %t540, i32 0, i32 1
  store i64* %t529, i64** %t542
  %t543 = load {i64, i64*}, {i64, i64*}* %t540
  store {i64, i64*} %t543, {i64, i64*}* %t527
  %t544 = alloca {i64, i64*}
  %t545 = load {i64, i64*}, {i64, i64*}* %t527
  %t546 = extractvalue {i64, i64*} %t545, 0
  %t547 = extractvalue {i64, i64*} %t545, 1
  %t548 = mul i64 %t546, 8
  %t549 = call i8* @malloc(i64 %t548)
  %t550 = bitcast i8* %t549 to i64*
  %t551 = alloca i64
  store i64 0, i64* %t551
  br label %L58

L58:
  %t552 = load i64, i64* %t551
  %t553 = icmp slt i64 %t552, %t546
  br i1 %t553, label %L59, label %L60

L59:
  %t554 = getelementptr i64, i64* %t547, i64 %t552
  %t555 = load i64, i64* %t554
  %t556 = alloca i64
  store i64 %t555, i64* %t556
  %t557 = load i64, i64* %t556
  %t559 = mul i64 %t557, 2
  %t560 = getelementptr i64, i64* %t550, i64 %t552
  store i64 %t559, i64* %t560
  %t561 = add i64 %t552, 1
  store i64 %t561, i64* %t551
  br label %L58

L60:
  %t562 = alloca {i64, i64*}
  %t563 = getelementptr {i64, i64*}, {i64, i64*}* %t562, i32 0, i32 0
  store i64 %t546, i64* %t563
  %t564 = getelementptr {i64, i64*}, {i64, i64*}* %t562, i32 0, i32 1
  store i64* %t550, i64** %t564
  %t565 = load {i64, i64*}, {i64, i64*}* %t562
  store {i64, i64*} %t565, {i64, i64*}* %t544
  %t566 = load {i64, i64*}, {i64, i64*}* %t544
  %t567 = call i64 @count({i64, i64*} %t566)
  %t569 = icmp eq i64 %t567, 5
  br i1 %t569, label %L61, label %L62

L61:
  %t570 = getelementptr [19 x i8], [19 x i8]* @.str.9, i32 0, i32 0
  call void @print_string(i8* %t570)
  ret i64 0
  br label %L63

L62:
  ret i64 1
  br label %L63

L63:
  ret i64 0
}

define i64 @run_all_tests() {
entry:
  %t573 = getelementptr [48 x i8], [48 x i8]* @.str.10, i32 0, i32 0
  call void @print_string(i8* %t573)
  %t574 = alloca i64
  %t575 = call i64 @test_literals_and_operators()
  store i64 %t575, i64* %t574
  %t576 = alloca i64
  %t577 = call i64 @test_data_and_postfix()
  store i64 %t577, i64* %t576
  %t578 = alloca i64
  %t580 = call i64 @test_if_else(i64 0)
  store i64 %t580, i64* %t578
  %t581 = alloca i64
  %t582 = call i64 @test_for_and_range()
  store i64 %t582, i64* %t581
  %t583 = alloca i64
  %t584 = call i64 @test_lambdas_and_pipelines()
  store i64 %t584, i64* %t583
  %t585 = alloca i64
  %t586 = call i64 @test_aggregates()
  store i64 %t586, i64* %t585
  %t587 = alloca i64
  %t588 = call i64 @test_dataframe_pipeline()
  store i64 %t588, i64* %t587
  %t589 = alloca i64
  %t590 = call i64 @test_assignment_and_logic()
  store i64 %t590, i64* %t589
  %t591 = alloca i64
  %t592 = call i64 @test_lambda_variable()
  store i64 %t592, i64* %t591
  %t593 = alloca i64
  %t594 = load i64, i64* %t574
  %t595 = load i64, i64* %t576
  %t596 = add i64 %t594, %t595
  %t597 = load i64, i64* %t578
  %t598 = add i64 %t596, %t597
  %t599 = load i64, i64* %t581
  %t600 = add i64 %t598, %t599
  %t601 = load i64, i64* %t583
  %t602 = add i64 %t600, %t601
  %t603 = load i64, i64* %t585
  %t604 = add i64 %t602, %t603
  %t605 = load i64, i64* %t587
  %t606 = add i64 %t604, %t605
  %t607 = load i64, i64* %t589
  %t608 = add i64 %t606, %t607
  %t609 = load i64, i64* %t591
  %t610 = add i64 %t608, %t609
  store i64 %t610, i64* %t593
  %t611 = getelementptr [28 x i8], [28 x i8]* @.str.11, i32 0, i32 0
  %t612 = load i64, i64* %t593
  call void @log_test(i8* %t611, i64 %t612)
  %t613 = getelementptr [38 x i8], [38 x i8]* @.str.12, i32 0, i32 0
  call void @print_string(i8* %t613)
  %t614 = load i64, i64* %t593
  call void @print_int(i64 %t614)
  %t615 = load i64, i64* %t593
  ret i64 %t615
}

define i64 @user_main() {
entry:
  call void @__init_globals()
  %t616 = getelementptr [56 x i8], [56 x i8]* @.str.13, i32 0, i32 0
  call void @print_string(i8* %t616)
  %t617 = getelementptr [11 x i8], [11 x i8]* @.str.14, i32 0, i32 0
  call void @print_string(i8* %t617)
  %t618 = load i64, i64* @global_globalInt
  call void @print_int(i64 %t618)
  %t619 = getelementptr [13 x i8], [13 x i8]* @.str.15, i32 0, i32 0
  call void @print_string(i8* %t619)
  %t620 = load double, double* @global_globalFloat
  call void @print_float(double %t620)
  %t621 = getelementptr [12 x i8], [12 x i8]* @.str.16, i32 0, i32 0
  call void @print_string(i8* %t621)
  %t622 = load i1, i1* @global_globalBool
  call void @print_bool(i1 %t622)
  %t623 = getelementptr [14 x i8], [14 x i8]* @.str.17, i32 0, i32 0
  call void @print_string(i8* %t623)
  %t624 = load i8*, i8** @global_globalString
  call void @print_string(i8* %t624)
  %t625 = getelementptr [16 x i8], [16 x i8]* @.str.18, i32 0, i32 0
  call void @print_string(i8* %t625)
  %t626 = load {i64, i64*}, {i64, i64*}* @global_globalArray
  %t628 = extractvalue {i64, i64*} %t626, 1
  %t629 = getelementptr i64, i64* %t628, i64 0
  %t630 = load i64, i64* %t629
  call void @print_int(i64 %t630)
  %t631 = alloca i64
  %t632 = call i64 @run_all_tests()
  store i64 %t632, i64* %t631
  %t633 = load i64, i64* %t631
  %t635 = icmp eq i64 %t633, 0
  br i1 %t635, label %L64, label %L65

L64:
  %t636 = getelementptr [35 x i8], [35 x i8]* @.str.19, i32 0, i32 0
  call void @print_string(i8* %t636)
  br label %L66

L65:
  %t637 = getelementptr [41 x i8], [41 x i8]* @.str.20, i32 0, i32 0
  call void @print_string(i8* %t637)
  br label %L66

L66:
  %t638 = load i64, i64* %t631
  ret i64 %t638
}

; ==================== STRING LITERALS ====================

@.str.0 = private constant [9 x i8] c"DataLang\00"
@.str.1 = private constant [14 x i8] c"Log de teste:\00"
@.str.2 = private constant [6 x i8] c"hello\00"
@.str.3 = private constant [6 x i8] c"Alice\00"
@.str.4 = private constant [28 x i8] c"Teste Person e indexação:\00"
@.str.5 = private constant [15 x i8] c"Muito negativo\00"
@.str.6 = private constant [17 x i8] c"Media dos pares:\00"
@.str.7 = private constant [13 x i8] c"Media total:\00"
@.str.8 = private constant [50 x i8] c"Teste DataFrame: SKIP (runtime não implementado)\00"
@.str.9 = private constant [19 x i8] c"Lambdas inline: OK\00"
@.str.10 = private constant [48 x i8] c"== Rodando suite completa de testes DataLang ==\00"
@.str.11 = private constant [28 x i8] c"Falhas totais (0 = tudo OK)\00"
@.str.12 = private constant [38 x i8] c"Numero total de falhas (0 = tudo OK):\00"
@.str.13 = private constant [56 x i8] c"==== Iniciando programa DataLang de teste completo ====\00"
@.str.14 = private constant [11 x i8] c"globalInt:\00"
@.str.15 = private constant [13 x i8] c"globalFloat:\00"
@.str.16 = private constant [12 x i8] c"globalBool:\00"
@.str.17 = private constant [14 x i8] c"globalString:\00"
@.str.18 = private constant [16 x i8] c"globalArray[0]:\00"
@.str.19 = private constant [35 x i8] c"SUCESSO: todos os testes passaram.\00"
@.str.20 = private constant [41 x i8] c"ERRO: existem falhas na suite de testes.\00"
