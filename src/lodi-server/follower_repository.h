#ifndef COSC522_LODI_FOLLOWER_REPOSITORY_H
#define COSC522_LODI_FOLLOWER_REPOSITORY_H
#include "collections/list.h"

void initFollowerRepository();

int addFollower(unsigned int idolId, unsigned int followerId);

int removeFollower(unsigned int idolId, unsigned int followerId);

int getFollowers(const unsigned int idolId, List **followers);
#endif
