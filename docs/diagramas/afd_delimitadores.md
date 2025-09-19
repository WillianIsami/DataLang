```mermaid
graph TD
    q0[q0] --> |(| q1((q1_LPAREN))
    q0 --> |)| q2((q2_RPAREN))
    q0 --> |[| q3((q3_LBRACKET))
    q0 --> |]| q4((q4_RBRACKET))
    q0 --> |{| q5((q5_LBRACE))
    q0 --> |}| q6((q6_RBRACE))
    q0 --> |;| q7((q7_SEMICOLON))
    q0 --> |,| q8((q8_COMMA))
    q0 --> |.| q9[q9]
    q9 --> |.| q10((q10_RANGE))
    q0 --> |:| q11((q11_COLON))
```