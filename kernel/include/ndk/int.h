#pragma once

#include <ndk/irql.h>

void softint_dispatch(irql_t newirql);
void softint_issue(irql_t irql);
void softint_ack(irql_t irql);
