/**
* Provides persistence for registered client ip addresses and ports
 **/

#include <string.h>

#include <stdio.h>
#include <stdlib.h>

#include "collections/list.h"
#include "collections/int_map.h"
#include "follower_repository.h"
#include "shared.h"

static IntMap *idolMap = NULL;

/**
 *  Constructor
 */
void initFollowerRepository() {
  createMap(&idolMap);
}

int addFollower(unsigned int idolId, unsigned int followerId) {
  if (!idolMap) {
    return ERROR;
  }
  List *followers = NULL;
  int rt = idolMap->get(idolMap, idolId, (void **) &followers);
  if (rt == ERROR) {
    return ERROR;
  }
  if (rt == NOT_FOUND) {
    if (createList(&followers) != SUCCESS) {
      return ERROR;
    }
    idolMap->add(idolMap, idolId, followers);
  }
  for (int i = 0; i < followers->length; i++) {
    int *follower = NULL;
    followers->get(followers, i, (void **) &follower);
    if (*follower == followerId) {
      printf("Warning - followerId=%u already added to follower list for idolId=%u\n",
             followerId, idolId);
      return SUCCESS;
    }
  }
  int *persistedFollowerId = malloc(sizeof(unsigned int));
  if (!persistedFollowerId) {
    return ERROR;
  }
  *persistedFollowerId = followerId;
  followers->append(followers, persistedFollowerId);
  return SUCCESS;
}

int removeFollower(unsigned int idolId, unsigned int followerId) {
  if (!idolMap) {
    return ERROR;
  }
  List *followers = NULL;
  int rt = idolMap->get(idolMap, idolId, (void **) &followers);
  if (rt == ERROR || rt == NOT_FOUND) {
    return rt;
  }
  for (int i = 0; i < followers->length; i++) {
    int *follower = NULL;
    followers->get(followers, i, (void **) &follower);
    if (*follower == followerId) {
      int * persistedFollowerId = NULL;
      followers->remove(followers, i, (void **) &persistedFollowerId);
      if (persistedFollowerId) {
        free(persistedFollowerId);
      } else {
        printf("Failed to retrieve id on removal, returning error...\n");
        return ERROR;
      }
      printf("Removed followerId=%u from list for idolId=%u\n",
             followerId, idolId);
      return SUCCESS;
    }
  }
  printf("Warning... followerId=%u not found for idolId=%u\n", followerId, idolId);
  return NOT_FOUND;
}


int getFollowers(const unsigned int idolId, List **followers) {
  if (!idolMap) {
    return ERROR;
  }
  const int rt = idolMap->get(idolMap, idolId, (void **) followers);
  if (rt == ERROR || rt == NOT_FOUND) {
    return rt;
  }
  if ((*followers)->length == 0) {
    return NOT_FOUND;
  }
  return SUCCESS;
}
